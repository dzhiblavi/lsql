#include "interface/sql/exec/plan.h"

#include "interface/sql/exec/GetFileSourceFunc.h"

#include "exec/expr/BinaryExpression.h"
#include "exec/expr/Coalesce.h"
#include "exec/expr/Expression.h"
#include "exec/expr/IdentifierExpression.h"
#include "exec/expr/UnaryAggregateExpression.h"
#include "exec/expr/UnaryExpression.h"
#include "exec/expr/ValueExpression.h"

#include "exec/op/Aggregate.h"
#include "exec/op/Filter.h"
#include "exec/op/Group.h"
#include "exec/op/In.h"
#include "exec/op/Limit.h"
#include "exec/op/Materialize.h"
#include "exec/op/MergeSorted.h"
#include "exec/op/Projection.h"
#include "exec/op/Sort.h"
#include "exec/op/UnionAll.h"
#include "exec/op/Values.h"

#include "interface/sql/bind/Expressions.h"
#include "interface/sql/bind/Relations.h"
#include "interface/sql/bind/Statement.h"

namespace lsql::iface::sql::exe {

namespace {

class Planner {
 public:
    Plan plan(bind::Program program) && {
        for (auto&& statement : program) {
            planStatement(std::move(statement));
        }
        return std::move(plan_);
    }

 private:
    void planStatement(bind::Statement s) {
        util::match(std::move(s), [this](auto s) { planStatement(std::move(s)); });
    }

    void planRelation(bind::Relation s) {
        util::match(std::move(s), [this](auto s) { planRelation(std::move(s)); });
    }

    void planExpr(bind::Expr s) {
        util::match(std::move(s), [this](auto s) { planExpr(std::move(s)); });
    }

    void planProjector(bind::Projector s) {
        util::match(std::move(s), [this](auto s) { planProjector(std::move(s)); });
    }

    void planStatement(bind::NamedRelationStatement s) {
        planRelation(std::move(*s.relation));
        named_ops[s.name] = popOperation();
    }

    void planStatement(bind::QueryStatement s) {
        planRelation(std::move(*s.relation));
        plan_.top_operations.push_back(popOperation());
    }

    void planRelation(bind::AdhocRelation r) {
        auto src = exec::values(std::move(r.values));
        operations.push(src);
        plan_.sources.push_back(src);
    }

    void planRelation(bind::SelectRelation r) {
        auto projectors = projectorsList(std::move(r.projectors));
        planRelation(std::move(*r.source));

        if (r.where) {
            planExpr(std::move(*r.where->condition));

            if (!exprs.empty()) {
                operations.push(exec::filter(popOperation(), popExpression()));
            }
        }

        if (r.group_by) {
            auto group_list = projectorsList(std::move(r.group_by->group_list));
            operations.push(
                exec::group(popOperation(), std::move(group_list), std::move(projectors)));
        } else {
            if (r.aggregate) {
                auto src = exec::aggregate(popOperation(), std::move(projectors));
                plan_.sources.push_back(src);
                operations.push(src);
            } else {
                operations.push(exec::projection(popOperation(), std::move(projectors)));
            }
        }

        if (r.order_by) {
            operations.push(
                exec::sort(
                    popOperation(),
                    expressionList(std::move(r.order_by->order_list)),
                    r.order_by->desc));
        }

        if (r.limit) {
            operations.push(exec::limit(popOperation(), r.limit->limit));
        }
    }

    void planRelation(bind::UnionAllRelation r) {
        planRelation(std::move(*r.left));
        auto left = popOperation();
        planRelation(std::move(*r.right));
        auto right = popOperation();
        operations.push(exec::unionAll(left, right));
    }

    void planRelation(bind::UnionAllSortedByRelation r) {
        planRelation(std::move(*r.left));
        auto left = popOperation();
        planRelation(std::move(*r.right));
        auto right = popOperation();
        auto order_list = expressionList(std::move(r.order_by.order_list));
        operations.push(exec::mergeSorted(left, right, std::move(order_list), r.order_by.desc));
    }

    void planRelation(bind::FileRelation r) {
        auto op = file_source_func_(r.path, std::nullopt);
        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planRelation(bind::FileIntervalRelation r) {
        auto op = file_source_func_(
            r.path,
            TimeRange{
                .ts_from = r.ts_from,
                .ts_to = r.ts_to,
            });

        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planRelation(bind::NamedRelationReferenceRelation r) {
        if (!named_ops.contains(r.name)) {
            throw std::runtime_error(std::format("no such named relation {}", r.name));
        }

        operations.push(named_ops[r.name]);
    }

    void planRelation(bind::MaterializeRelation r) {
        planRelation(std::move(*r.relation));
        auto op = exec::materialize(popOperation());
        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planExpr(bind::FieldExpr e) {
        exprs.push_back(std::make_shared<exec::IdentifierExpression>(e.identifier));
    }

    void planExpr(bind::ValueExpr e) {
        exprs.push_back(std::make_shared<exec::ValueExpression>(e.value));
    }

    void planExpr(bind::InExpr e) {
        auto source = popOperation();
        planExpr(std::move(*e.expr));
        auto expr = popExpression();
        planRelation(std::move(*e.source));
        auto match = popOperation();
        operations.push(exec::in(source, match, expr));
    }

    void planExpr(bind::CoalesceExpr e) {
        exprs.push_back(std::make_shared<exec::Coalesce>(expressionList(std::move(e.args))));
    }

    void planExpr(bind::CastExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::CastOp>>(
                arg, arg->valueType(), e.cast_to));
    }

    void planExpr(bind::PercentileExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(
            std::make_shared<exec::UnaryAggregateExpression<exec::PercentileOp>>(
                arg, std::move(e.percentiles), arg->valueType()));
    }

    void planExpr(bind::LikeExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(std::make_shared<exec::UnaryExpression<exec::LikeOp>>(arg, e.regex));
    }

    void planExpr(bind::RSubstrExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(std::make_shared<exec::UnaryExpression<exec::RSubstrOp>>(arg, e.regex));
    }

    void planExpr(bind::UnaryExpr e) {
        planExpr(std::move(*e.expr));

        switch (e.type) {
            case bind::UnaryExprType::BooleanNegate:
                exprs.push_back(
                    std::make_shared<exec::UnaryExpression<exec::BooleanNegationOp>>(
                        popExpression()));
        }
    }

    void planExpr(bind::UnaryAggregateExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case bind::UnaryAggregateExprType::Count:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::CountOp>>(arg);
                case bind::UnaryAggregateExprType::Min:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MinOp>>(
                        arg, e.valueType());
                case bind::UnaryAggregateExprType::Max:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MaxOp>>(
                        arg, e.valueType());
                case bind::UnaryAggregateExprType::Sum:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::SumOp>>(
                        arg, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void planExpr(bind::BinaryExpr e) {
        planExpr(std::move(*e.left));
        auto l = popExpression();
        planExpr(std::move(*e.right));
        auto r = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case bind::BinaryExprType::Equal:
                    return std::make_shared<exec::BinaryExpression<exec::EqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case bind::BinaryExprType::NotEqual:
                    return std::make_shared<exec::BinaryExpression<exec::NotEqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case bind::BinaryExprType::And:
                    return std::make_shared<exec::BinaryExpression<exec::AndOp>>(l, r);
                case bind::BinaryExprType::Or:
                    return std::make_shared<exec::BinaryExpression<exec::OrOp>>(l, r);
                case bind::BinaryExprType::Divide:
                    return std::make_shared<exec::BinaryExpression<exec::DivideOp>>(
                        l, r, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void planProjector(bind::StarProjector) {
        projectors.push_back(std::make_unique<exec::Projector>("", nullptr));
    }

    void planProjector(bind::ExprProjector p) {
        planExpr(std::move(*p.expr));
        projectors.push_back(std::make_unique<exec::Projector>(p.alias, popExpression()));
    }

    std::shared_ptr<exec::Expression> popExpression() {
        verify(!exprs.empty());
        auto top = exprs.back();
        exprs.pop_back();
        return top;
    }

    exec::OperationPtr popOperation() {
        verify(!operations.empty());
        auto top = operations.top();
        operations.pop();
        return top;
    }

    std::vector<exec::ExpressionPtr> expressionList(std::vector<bind::Expr> list) {
        verify(exprs.empty());
        for (auto&& item : list) {
            planExpr(std::move(item));
        }
        verify(exprs.size() == list.size());
        return std::move(exprs);
    }

    exec::ProjectionList projectorsList(std::vector<bind::Projector> list) {
        verify(projectors.empty());
        for (auto&& item : list) {
            planProjector(std::move(item));
        }
        verify(projectors.size() == list.size());
        return std::move(projectors);
    }

    exec::ProjectionList projectors;
    std::vector<exec::ExpressionPtr> exprs;

    std::unordered_map<std::string, exec::OperationPtr> named_ops;
    std::stack<exec::OperationPtr> operations;

    Plan plan_;

    GetFileSourceFuncType file_source_func_ = defaultFileSourceFunc();
};

}  // namespace

Plan plan(bind::Program program) {
    return Planner().plan(std::move(program));
}

}  // namespace lsql::iface::sql::exe

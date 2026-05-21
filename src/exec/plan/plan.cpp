#include "exec/plan/plan.h"

#include "exec/plan/GetFileSourceFunc.h"

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

#include "ir/Expressions.h"
#include "ir/Relations.h"
#include "ir/Statement.h"

namespace lsql::exec {

namespace {

class Planner {
 public:
    Plan plan(ir::Program program) && {
        binding_ = program.field_binding;
        plan_.field_binding = binding_;
        for (auto&& statement : program.statements) {
            planStatement(std::move(statement));
        }
        return std::move(plan_);
    }

 private:
    void planStatement(ir::Statement s) {
        util::match(std::move(s), [this](auto s) { planStatement(std::move(s)); });
    }

    void planRelation(ir::Relation s) {
        util::match(std::move(s), [this](auto s) { planRelation(std::move(s)); });
    }

    void planExpr(ir::Expr s) {
        util::match(std::move(s), [this](auto s) { planExpr(std::move(s)); });
    }

    void planProjector(ir::Projector s) {
        util::match(std::move(s), [this](auto s) { planProjector(std::move(s)); });
    }

    void planStatement(ir::NamedRelationStatement s) {
        planRelation(std::move(*s.relation));
        named_ops[s.name] = popOperation();
    }

    void planStatement(ir::QueryStatement s) {
        planRelation(std::move(*s.relation));
        plan_.top_operations.push_back(popOperation());
    }

    void planRelation(ir::ValuesRelation r) {
        auto src = exec::values(std::move(r.values), binding_);
        operations.push(src);
        plan_.sources.push_back(src);
    }

    void planRelation(ir::ProjectionRelation r) {
        auto projectors = projectorsList(std::move(r.projectors));
        planRelation(std::move(*r.source));
        operations.push(exec::projection(popOperation(), std::move(projectors), binding_));
    }

    void planRelation(ir::AggregateRelation r) {
        auto projectors = projectorsList(std::move(r.projectors));
        planRelation(std::move(*r.source));
        operations.push(exec::aggregate(popOperation(), std::move(projectors), binding_));
    }

    void planRelation(ir::GroupRelation r) {
        auto projectors = projectorsList(std::move(r.projectors));
        auto group_list = projectorsList(std::move(r.group_list));
        planRelation(std::move(*r.source));
        operations.push(
            exec::group(popOperation(), std::move(group_list), std::move(projectors), binding_));
    }

    void planRelation(ir::LimitRelation r) {
        planRelation(std::move(*r.source));
        operations.push(exec::limit(popOperation(), r.limit, binding_));
    }

    void planRelation(ir::FilterRelation r) {
        planRelation(std::move(*r.source));
        planExpr(std::move(*r.condition));

        if (!exprs.empty()) {
            operations.push(exec::filter(popOperation(), popExpression(), binding_));
        }
    }

    void planRelation(ir::SortRelation r) {
        planRelation(std::move(*r.source));
        operations.push(
            exec::sort(popOperation(), expressionList(std::move(r.order_list)), r.desc, binding_));
    }

    void planRelation(ir::SemiJoinRelation r) {
        planRelation(std::move(*r.source));
        auto source = popOperation();

        planRelation(std::move(*r.match));
        auto match = popOperation();

        planExpr(std::move(*r.expr));
        auto expr = popExpression();

        operations.push(exec::in(source, match, expr, binding_));
    }

    void planRelation(ir::UnionAllRelation r) {
        planRelation(std::move(*r.left));
        auto left = popOperation();
        planRelation(std::move(*r.right));
        auto right = popOperation();
        operations.push(exec::unionAll(left, right, binding_));
    }

    void planRelation(ir::UnionAllSortedByRelation r) {
        planRelation(std::move(*r.left));
        auto left = popOperation();
        planRelation(std::move(*r.right));
        auto right = popOperation();
        auto order_list = expressionList(std::move(r.order_list));
        operations.push(exec::mergeSorted(left, right, std::move(order_list), r.desc, binding_));
    }

    void planRelation(ir::FileRelation r) {
        auto op = file_source_func_(r.path, binding_, std::nullopt);
        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planRelation(ir::FileIntervalRelation r) {
        auto op = file_source_func_(
            r.path,
            binding_,
            TimeRange{
                .ts_from = r.ts_from,
                .ts_to = r.ts_to,
            });

        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planRelation(ir::NamedRelationReferenceRelation r) {
        if (!named_ops.contains(r.name)) {
            throw std::runtime_error(std::format("no such named relation {}", r.name));
        }

        operations.push(named_ops[r.name]);
    }

    void planRelation(ir::MaterializeRelation r) {
        planRelation(std::move(*r.relation));
        auto op = exec::materialize(popOperation(), binding_);
        plan_.sources.push_back(op);
        operations.push(op);
    }

    void planExpr(ir::FieldExpr e) {
        exprs.push_back(std::make_shared<exec::IdentifierExpression>(e.field_id));
    }

    void planExpr(ir::ValueExpr e) {
        exprs.push_back(std::make_shared<exec::ValueExpression>(e.value));
    }

    void planExpr(ir::CoalesceExpr e) {
        exprs.push_back(std::make_shared<exec::Coalesce>(expressionList(std::move(e.args))));
    }

    void planExpr(ir::CastExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(
            std::make_shared<exec::UnaryExpression<exec::CastOp>>(
                arg, arg->valueType(), e.cast_to));
    }

    void planExpr(ir::PercentileExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(
            std::make_shared<exec::UnaryAggregateExpression<exec::PercentileOp>>(
                arg, std::move(e.percentiles), arg->valueType()));
    }

    void planExpr(ir::LikeExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(std::make_shared<exec::UnaryExpression<exec::LikeOp>>(arg, e.regex));
    }

    void planExpr(ir::RSubstrExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();
        exprs.push_back(std::make_shared<exec::UnaryExpression<exec::RSubstrOp>>(arg, e.regex));
    }

    void planExpr(ir::UnaryExpr e) {
        planExpr(std::move(*e.expr));

        switch (e.type) {
            case ir::UnaryExprType::BooleanNegate:
                exprs.push_back(
                    std::make_shared<exec::UnaryExpression<exec::BooleanNegationOp>>(
                        popExpression()));
        }
    }

    void planExpr(ir::UnaryAggregateExpr e) {
        planExpr(std::move(*e.expr));
        auto arg = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case ir::UnaryAggregateExprType::Count:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::CountOp>>(arg);
                case ir::UnaryAggregateExprType::Min:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MinOp>>(
                        arg, e.valueType());
                case ir::UnaryAggregateExprType::Max:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::MaxOp>>(
                        arg, e.valueType());
                case ir::UnaryAggregateExprType::Sum:
                    return std::make_shared<exec::UnaryAggregateExpression<exec::SumOp>>(
                        arg, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void planExpr(ir::BinaryExpr e) {
        planExpr(std::move(*e.left));
        auto l = popExpression();
        planExpr(std::move(*e.right));
        auto r = popExpression();

        auto expr = [&] -> exec::ExpressionPtr {
            switch (e.type) {
                case ir::BinaryExprType::Equal:
                    return std::make_shared<exec::BinaryExpression<exec::EqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case ir::BinaryExprType::NotEqual:
                    return std::make_shared<exec::BinaryExpression<exec::NotEqualOp>>(
                        l, r, l->valueType(), r->valueType());
                case ir::BinaryExprType::And:
                    return std::make_shared<exec::BinaryExpression<exec::AndOp>>(l, r);
                case ir::BinaryExprType::Or:
                    return std::make_shared<exec::BinaryExpression<exec::OrOp>>(l, r);
                case ir::BinaryExprType::Divide:
                    return std::make_shared<exec::BinaryExpression<exec::DivideOp>>(
                        l, r, e.valueType());
            }
        }();

        exprs.push_back(expr);
    }

    void planProjector(ir::StarProjector) {
        projectors.push_back(std::make_unique<exec::Projector>(0, nullptr));
    }

    void planProjector(ir::ExprProjector p) {
        planExpr(std::move(*p.expr));
        projectors.push_back(std::make_unique<exec::Projector>(p.alias_field_id, popExpression()));
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

    std::vector<exec::ExpressionPtr> expressionList(std::vector<ir::Expr> list) {
        verify(exprs.empty());
        for (auto&& item : list) {
            planExpr(std::move(item));
        }
        verify(exprs.size() == list.size());
        return std::move(exprs);
    }

    exec::ProjectionList projectorsList(std::vector<ir::Projector> list) {
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
    ConstFieldBindingPtr binding_;
};

}  // namespace

Plan plan(ir::Program program) {
    return Planner().plan(std::move(program));
}

}  // namespace lsql::exec

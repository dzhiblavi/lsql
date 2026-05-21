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
#include "exec/op/Limit.h"
#include "exec/op/MarkJoin.h"
#include "exec/op/Materialize.h"
#include "exec/op/MergeSorted.h"
#include "exec/op/Projection.h"
#include "exec/op/SemiJoin.h"
#include "exec/op/Sort.h"
#include "exec/op/UnionAll.h"
#include "exec/op/Values.h"

#include "ir/Expressions.h"
#include "ir/Relations.h"
#include "ir/Statement.h"

#include "core/require.h"

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

    OperationPtr planRelation(ir::Relation s) {
        return util::match(std::move(s), [this](auto s) { return planRelation(std::move(s)); });
    }

    ExpressionPtr planExpr(ir::Expr s) {
        return util::match(std::move(s), [this](auto s) { return planExpr(std::move(s)); });
    }

    ProjectorPtr planProjector(ir::Projector s) {
        return util::match(std::move(s), [this](auto s) { return planProjector(std::move(s)); });
    }

    void planStatement(ir::NamedRelationStatement s) {
        auto op = planRelation(std::move(*s.relation));
        named_ops[s.name] = op;
    }

    void planStatement(ir::QueryStatement s) {
        plan_.top_operations.push_back(planRelation(std::move(*s.relation)));
    }

    OperationPtr planRelation(ir::ValuesRelation r) {
        auto src = values(std::move(r.values), r.output_id, binding_);
        plan_.sources.push_back(src);
        return src;
    }

    OperationPtr planRelation(ir::ProjectionRelation r) {
        return projection(
            planRelation(std::move(*r.source)), projectorsList(std::move(r.projectors)), binding_);
    }

    OperationPtr planRelation(ir::AggregateRelation r) {
        return aggregate(
            planRelation(std::move(*r.source)), projectorsList(std::move(r.projectors)), binding_);
    }

    OperationPtr planRelation(ir::GroupRelation r) {
        return group(
            planRelation(std::move(*r.source)),
            projectorsList(std::move(r.group_list)),
            projectorsList(std::move(r.projectors)),
            binding_);
    }

    OperationPtr planRelation(ir::LimitRelation r) {
        return limit(planRelation(std::move(*r.source)), r.limit, binding_);
    }

    OperationPtr planRelation(ir::FilterRelation r) {
        return filter(
            planRelation(std::move(*r.source)), planExpr(std::move(*r.condition)), binding_);
    }

    OperationPtr planRelation(ir::SortRelation r) {
        return sort(
            planRelation(std::move(*r.source)),
            expressionList(std::move(r.order_list)),
            r.desc,
            binding_);
    }

    OperationPtr planRelation(ir::SemiJoinRelation r) {
        return semiJoin(
            planRelation(std::move(*r.source)),
            planRelation(std::move(*r.match)),
            planExpr(std::move(*r.expr)),
            binding_);
    }

    OperationPtr planRelation(ir::MarkJoinRelation r) {
        return markJoin(
            planRelation(std::move(*r.source)),
            planRelation(std::move(*r.match)),
            planExpr(std::move(*r.expr)),
            r.output_field_id,
            binding_);
    }

    OperationPtr planRelation(ir::UnionAllRelation r) {
        return unionAll(
            planRelation(std::move(*r.left)), planRelation(std::move(*r.right)), binding_);
    }

    OperationPtr planRelation(ir::UnionAllSortedByRelation r) {
        return mergeSorted(
            planRelation(std::move(*r.left)),
            planRelation(std::move(*r.right)),
            expressionList(std::move(r.order_list)),
            r.desc,
            binding_);
    }

    OperationPtr planRelation(ir::FileRelation r) {
        auto src = file_source_func_(r.path, binding_, std::nullopt);
        plan_.sources.push_back(src);
        return src;
    }

    OperationPtr planRelation(ir::FileIntervalRelation r) {
        auto src = file_source_func_(
            r.path,
            binding_,
            TimeRange{
                .ts_from = r.ts_from,
                .ts_to = r.ts_to,
            });

        plan_.sources.push_back(src);
        return src;
    }

    OperationPtr planRelation(ir::NamedRelationReferenceRelation r) {
        require(named_ops.contains(r.name), "no such named relation {}", r.name);
        return named_ops[r.name];
    }

    OperationPtr planRelation(ir::MaterializeRelation r) {
        auto src = materialize(planRelation(std::move(*r.relation)), binding_);
        plan_.sources.push_back(src);
        return src;
    }

    ExpressionPtr planExpr(ir::FieldExpr e) {
        return std::make_shared<IdentifierExpression>(e.field_id, e.type);
    }

    ExpressionPtr planExpr(ir::ValueExpr e) { return std::make_shared<ValueExpression>(e.value); }

    ExpressionPtr planExpr(ir::CoalesceExpr e) {
        return std::make_shared<Coalesce>(expressionList(std::move(e.args)));
    }

    ExpressionPtr planExpr(ir::CastExpr e) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<CastOp>>(arg, arg->valueType(), e.cast_to);
    }

    ExpressionPtr planExpr(ir::PercentileExpr e) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryAggregateExpression<PercentileOp>>(
            arg, std::move(e.percentiles), arg->valueType());
    }

    ExpressionPtr planExpr(ir::LikeExpr e) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<LikeOp>>(arg, e.regex);
    }

    ExpressionPtr planExpr(ir::RSubstrExpr e) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<RSubstrOp>>(arg, e.regex);
    }

    ExpressionPtr planExpr(ir::UnaryExpr e) {
        auto arg = planExpr(std::move(*e.expr));

        switch (e.type) {
            case ir::UnaryExprType::BooleanNegate:
                return std::make_shared<UnaryExpression<BooleanNegationOp>>(arg);
        }
    }

    ExpressionPtr planExpr(ir::UnaryAggregateExpr e) {
        auto arg = planExpr(std::move(*e.expr));

        switch (e.type) {
            case ir::UnaryAggregateExprType::Count:
                return std::make_shared<UnaryAggregateExpression<CountOp>>(arg);
            case ir::UnaryAggregateExprType::Min:
                return std::make_shared<UnaryAggregateExpression<MinOp>>(arg, e.valueType());
            case ir::UnaryAggregateExprType::Max:
                return std::make_shared<UnaryAggregateExpression<MaxOp>>(arg, e.valueType());
            case ir::UnaryAggregateExprType::Sum:
                return std::make_shared<UnaryAggregateExpression<SumOp>>(arg, e.valueType());
        }
    }

    ExpressionPtr planExpr(ir::BinaryExpr e) {
        auto l = planExpr(std::move(*e.left));
        auto r = planExpr(std::move(*e.right));

        switch (e.type) {
            case ir::BinaryExprType::Equal:
                return std::make_shared<BinaryExpression<EqualOp>>(
                    l, r, l->valueType(), r->valueType());
            case ir::BinaryExprType::NotEqual:
                return std::make_shared<BinaryExpression<NotEqualOp>>(
                    l, r, l->valueType(), r->valueType());
            case ir::BinaryExprType::And:
                return std::make_shared<BinaryExpression<AndOp>>(l, r);
            case ir::BinaryExprType::Or:
                return std::make_shared<BinaryExpression<OrOp>>(l, r);
            case ir::BinaryExprType::Divide:
                return std::make_shared<BinaryExpression<DivideOp>>(l, r, e.valueType());
        }
    }

    ProjectorPtr planProjector(ir::StarProjector) {
        return std::make_unique<Projector>(0, nullptr);
    }

    ProjectorPtr planProjector(ir::ExprProjector p) {
        return std::make_unique<Projector>(p.alias_field_id, planExpr(std::move(*p.expr)));
    }

    std::vector<ExpressionPtr> expressionList(std::vector<ir::Expr> list) {
        std::vector<ExpressionPtr> exprs;
        exprs.reserve(list.size());
        for (auto&& item : list) {
            exprs.push_back(planExpr(std::move(item)));
        }
        return exprs;
    }

    ProjectionList projectorsList(std::vector<ir::Projector> list) {
        ProjectionList proj;
        proj.reserve(list.size());
        for (auto&& item : list) {
            proj.push_back(planProjector(std::move(item)));
        }
        return proj;
    }

    Plan plan_;
    std::unordered_map<std::string, OperationPtr> named_ops;
    ConstFieldBindingPtr binding_;
    GetFileSourceFuncType file_source_func_ = defaultFileSourceFunc();
};

}  // namespace

Plan plan(ir::Program program) {
    return Planner().plan(std::move(program));
}

}  // namespace lsql::exec

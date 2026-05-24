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

#include "ir/Aggregates.h"
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
        return util::match(
            std::move(s.node), [&](auto node) { return planRelation(std::move(node), s); });
    }

    ExpressionPtr planExpr(ir::Expr s) {
        return util::match(
            std::move(s.node), [&](auto node) { return planExpr(std::move(node), s); });
    }

    AggregateProjectorPtr planAggregate(ir::Aggregate s) {
        return util::match(
            std::move(s.node), [&](auto node) { return planAggregate(std::move(node), s); });
    }

    void planStatement(ir::NamedRelationStatement s) {
        auto op = planRelation(std::move(*s.relation));
        named_ops[s.name] = op;
    }

    void planStatement(ir::QueryStatement s) {
        auto fields = s.relation->fields_out;
        auto r = planRelation(std::move(*s.relation));
        plan_.top_operations.emplace_back(std::move(r), fields);
    }

    OperationPtr planRelation(ir::ValuesRelation r, auto& /*info*/) {
        auto src = values(std::move(r.values), r.output_id, binding_);
        plan_.sources.push_back(src);
        return src;
    }

    OperationPtr planRelation(ir::ProjectionRelation r, auto& /*info*/) {
        return projection(
            planRelation(std::move(*r.source)), projectorsList(std::move(r.projectors)), binding_);
    }

    OperationPtr planRelation(ir::AggregateRelation r, auto& /*info*/) {
        return aggregate(
            planRelation(std::move(*r.source)), aggregatesList(std::move(r.aggregates)), binding_);
    }

    OperationPtr planRelation(ir::GroupRelation r, auto& /*info*/) {
        return group(
            planRelation(std::move(*r.source)),
            aggregatesList(std::move(r.aggregates)),
            projectorsList(std::move(r.group_list)),
            binding_);
    }

    OperationPtr planRelation(ir::LimitRelation r, auto& /*info*/) {
        return limit(planRelation(std::move(*r.source)), r.limit, binding_);
    }

    OperationPtr planRelation(ir::FilterRelation r, auto& /*info*/) {
        return filter(
            planRelation(std::move(*r.source)), planExpr(std::move(*r.condition)), binding_);
    }

    OperationPtr planRelation(ir::SortRelation r, auto& /*info*/) {
        return sort(
            planRelation(std::move(*r.source)),
            expressionList(std::move(r.order_list)),
            r.desc,
            binding_);
    }

    OperationPtr planRelation(ir::SemiJoinRelation r, auto& /*info*/) {
        return semiJoin(
            planRelation(std::move(*r.source)),
            planRelation(std::move(*r.match)),
            planExpr(std::move(*r.expr)),
            r.match_field_id,
            binding_);
    }

    OperationPtr planRelation(ir::MarkJoinRelation r, auto& /*info*/) {
        return markJoin(
            planRelation(std::move(*r.source)),
            planRelation(std::move(*r.match)),
            planExpr(std::move(*r.expr)),
            r.output_field_id,
            r.match_field_id,
            binding_);
    }

    OperationPtr planRelation(ir::UnionAllRelation r, auto& /*info*/) {
        return unionAll(
            planRelation(std::move(*r.left)), planRelation(std::move(*r.right)), binding_);
    }

    OperationPtr planRelation(ir::UnionAllSortedByRelation r, auto& /*info*/) {
        return mergeSorted(
            planRelation(std::move(*r.left)),
            planRelation(std::move(*r.right)),
            expressionList(std::move(r.order_list)),
            r.desc,
            binding_);
    }

    OperationPtr planRelation(ir::FileRelation r, auto& /*info*/) {
        auto src = file_source_func_(r.path, binding_, std::nullopt);
        plan_.sources.push_back(src);
        return src;
    }

    OperationPtr planRelation(ir::FileIntervalRelation r, auto& /*info*/) {
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

    OperationPtr planRelation(ir::NamedRelationReferenceRelation r, auto& /*info*/) {
        require(named_ops.contains(r.name), "no such named relation {}", r.name);
        return named_ops[r.name];
    }

    OperationPtr planRelation(ir::MaterializeRelation r, auto& /*info*/) {
        auto src = materialize(planRelation(std::move(*r.relation)), binding_);
        plan_.sources.push_back(src);
        return src;
    }

    ExpressionPtr planExpr(ir::FieldExpr e, auto& info) {
        return std::make_shared<IdentifierExpression>(e.field_id, info.value_type);
    }

    ExpressionPtr planExpr(ir::ValueExpr e, auto& /*info*/) {
        return std::make_shared<ValueExpression>(e.value);
    }

    ExpressionPtr planExpr(ir::CoalesceExpr e, auto& /*info*/) {
        return std::make_shared<Coalesce>(expressionList(std::move(e.args)));
    }

    ExpressionPtr planExpr(ir::CastExpr e, auto& /*info*/) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<CastOp>>(arg, arg->valueType(), e.cast_to);
    }

    ExpressionPtr planExpr(ir::LikeExpr e, auto& /*info*/) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<LikeOp>>(arg, e.regex);
    }

    ExpressionPtr planExpr(ir::RSubstrExpr e, auto& /*info*/) {
        auto arg = planExpr(std::move(*e.expr));
        return std::make_shared<UnaryExpression<RSubstrOp>>(arg, e.regex);
    }

    ExpressionPtr planExpr(ir::UnaryExpr e, auto& /*info*/) {
        auto arg = planExpr(std::move(*e.expr));

        switch (e.type) {
            case UnaryExprType::BooleanNegate:
                return std::make_shared<UnaryExpression<BooleanNegationOp>>(arg);
        }
    }

    ExpressionPtr planExpr(ir::BinaryExpr e, auto& info) {
        auto l = planExpr(std::move(*e.left));
        auto r = planExpr(std::move(*e.right));

        switch (e.type) {
            case BinaryExprType::Equal:
                return std::make_shared<BinaryExpression<EqualOp>>(
                    l, r, l->valueType(), r->valueType());
            case BinaryExprType::NotEqual:
                return std::make_shared<BinaryExpression<NotEqualOp>>(
                    l, r, l->valueType(), r->valueType());
            case BinaryExprType::And:
                return std::make_shared<BinaryExpression<AndOp>>(l, r);
            case BinaryExprType::Or:
                return std::make_shared<BinaryExpression<OrOp>>(l, r);
            case BinaryExprType::Divide:
                return std::make_shared<BinaryExpression<DivideOp>>(l, r, info.value_type);
        }
    }

    AggregateProjectorPtr planAggregate(ir::ScalarAggregate a, auto&& info) {
        auto arg = planExpr(std::move(*a.expr));
        auto aggregate = [&] -> AggregatePtr {
            switch (a.type) {
                case UnaryAggregateExprType::Count:
                    return std::make_shared<UnaryAggregateExpression<CountOp>>(arg);
                case UnaryAggregateExprType::Min:
                    return std::make_shared<UnaryAggregateExpression<MinOp>>(arg, info.value_type);
                case UnaryAggregateExprType::Max:
                    return std::make_shared<UnaryAggregateExpression<MaxOp>>(arg, info.value_type);
                case UnaryAggregateExprType::Sum:
                    return std::make_shared<UnaryAggregateExpression<SumOp>>(arg, info.value_type);
            }
        }();

        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = aggregate,
            });
    }

    AggregateProjectorPtr planAggregate(ir::PercentileAggregate a, auto&& info) {
        auto arg = planExpr(std::move(*a.expr));
        auto aggregate = arc<UnaryAggregateExpression<PercentileOp>>(
            arg, std::move(a.percentiles), arg->valueType());

        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = std::move(aggregate),
            });
    }

    ScalarProjectorPtr planProjector(ir::Projector p) {
        return std::make_unique<ScalarProjector>(p.alias_field_id, planExpr(std::move(*p.expr)));
    }

    std::vector<ExpressionPtr> expressionList(std::vector<ir::Expr> list) {
        std::vector<ExpressionPtr> exprs;
        exprs.reserve(list.size());
        for (auto&& item : list) {
            exprs.push_back(planExpr(std::move(item)));
        }
        return exprs;
    }

    ScalarProjectionList projectorsList(std::vector<ir::Projector> list) {
        ScalarProjectionList proj;
        proj.reserve(list.size());
        for (auto&& item : list) {
            proj.push_back(planProjector(std::move(item)));
        }
        return proj;
    }

    AggregateProjectionList aggregatesList(std::vector<ir::Aggregate> list) {
        AggregateProjectionList proj;
        proj.reserve(list.size());
        for (auto&& item : list) {
            proj.push_back(planAggregate(std::move(item)));
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

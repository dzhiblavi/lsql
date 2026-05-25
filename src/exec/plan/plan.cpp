#include "exec/plan/plan.h"

#include "exec/plan/GetFileSourceFunc.h"

#include "exec/expr/BinaryScalar.h"
#include "exec/expr/CoalesceScalar.h"
#include "exec/expr/IdentifierScalar.h"
#include "exec/expr/UnaryAggregate.h"
#include "exec/expr/UnaryScalar.h"
#include "exec/expr/ValueScalar.h"

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
#include "ir/Relations.h"
#include "ir/Scalars.h"
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

    ScalarPtr planScalar(ir::Scalar s) {
        return util::match(
            std::move(s.node), [&](auto node) { return planScalar(std::move(node), s); });
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
            planRelation(std::move(*r.source)), planScalar(std::move(*r.condition)), binding_);
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
            planScalar(std::move(*r.expr)),
            r.match_field_id,
            binding_);
    }

    OperationPtr planRelation(ir::MarkJoinRelation r, auto& /*info*/) {
        return markJoin(
            planRelation(std::move(*r.source)),
            planRelation(std::move(*r.match)),
            planScalar(std::move(*r.expr)),
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

    ScalarPtr planScalar(ir::FieldScalar e, auto& info) {
        return arc<IdentifierScalar>(e.field_id, info.value_type);
    }

    ScalarPtr planScalar(ir::ValueScalar e, auto& /*info*/) { return arc<ValueScalar>(e.value); }

    ScalarPtr planScalar(ir::CoalesceScalar e, auto& /*info*/) {
        return arc<CoalesceScalar>(expressionList(std::move(e.args)));
    }

    ScalarPtr planScalar(ir::CastScalar e, auto& /*info*/) {
        auto arg = planScalar(std::move(*e.expr));
        return arc<UnaryScalar<CastOp>>(arg, arg->valueType(), e.cast_to);
    }

    ScalarPtr planScalar(ir::LikeScalar e, auto& /*info*/) {
        auto arg = planScalar(std::move(*e.expr));
        return arc<UnaryScalar<LikeOp>>(arg, e.regex);
    }

    ScalarPtr planScalar(ir::RSubstrScalar e, auto& /*info*/) {
        auto arg = planScalar(std::move(*e.expr));
        return arc<UnaryScalar<RSubstrOp>>(arg, e.regex);
    }

    ScalarPtr planScalar(ir::UnaryScalar e, auto& /*info*/) {
        auto arg = planScalar(std::move(*e.expr));

        switch (e.type) {
            case UnaryExprType::BooleanNegate:
                return arc<UnaryScalar<BooleanNegationOp>>(arg);
        }
    }

    ScalarPtr planScalar(ir::BinaryScalar e, auto& info) {
        auto l = planScalar(std::move(*e.left));
        auto r = planScalar(std::move(*e.right));

        switch (e.type) {
            case BinaryExprType::Equal:
                return arc<BinaryScalar<EqualOp>>(l, r, l->valueType(), r->valueType());
            case BinaryExprType::NotEqual:
                return arc<BinaryScalar<NotEqualOp>>(l, r, l->valueType(), r->valueType());
            case BinaryExprType::And:
                return arc<BinaryScalar<AndOp>>(l, r);
            case BinaryExprType::Or:
                return arc<BinaryScalar<OrOp>>(l, r);
            case BinaryExprType::Divide:
                return dispatch<ScalarPtr>(
                    [&]<Dividable T>(std::type_identity<T>) -> ScalarPtr {
                        return arc<BinaryScalar<DivideOp<T>>>(l, r, info.value_type);
                    },
                    info.value_type);
            case BinaryExprType::Add:
                return dispatch<ScalarPtr>(
                    [&]<Addable T>(std::type_identity<T>) -> ScalarPtr {
                        return arc<BinaryScalar<AddOp<T>>>(l, r, info.value_type);
                    },
                    info.value_type);
            case BinaryExprType::Subtract:
                return dispatch<ScalarPtr>(
                    [&]<Subtractable T>(std::type_identity<T>) -> ScalarPtr {
                        return arc<BinaryScalar<SubtractOp<T>>>(l, r, info.value_type);
                    },
                    info.value_type);
        }
    }

    AggregateProjectorPtr planAggregate(ir::UnaryAggregate a, auto&& info) {
        auto arg = planScalar(std::move(*a.expr));
        auto aggregate = [&] -> AggregatePtr {
            switch (a.type) {
                case UnaryAggregateType::Count:
                    return arc<UnaryAggregate<CountOp>>(arg);
                case UnaryAggregateType::Min:
                    return dispatch<AggregatePtr>(
                        [&]<Comparable T>(std::type_identity<T>) -> AggregatePtr {
                            return arc<UnaryAggregate<MinOp<T>>>(arg, info.value_type);
                        },
                        info.value_type);
                case UnaryAggregateType::Max:
                    return dispatch<AggregatePtr>(
                        [&]<Comparable T>(std::type_identity<T>) -> AggregatePtr {
                            return arc<UnaryAggregate<MaxOp<T>>>(arg, info.value_type);
                        },
                        info.value_type);
                case UnaryAggregateType::Sum:
                    return dispatch<AggregatePtr>(
                        [&]<Addable T>(std::type_identity<T>) -> AggregatePtr {
                            return arc<UnaryAggregate<SumOp<T>>>(arg, info.value_type);
                        },
                        info.value_type);
            }
        }();

        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = aggregate,
            });
    }

    AggregateProjectorPtr planAggregate(ir::PercentileAggregate a, auto&& info) {
        auto arg = planScalar(std::move(*a.expr));

        auto aggregate = dispatch<AggregatePtr>(
            [&]<Comparable T>(std::type_identity<T>) -> AggregatePtr {
                return arc<UnaryAggregate<PercentileOp<T>>>(
                    arg, std::move(a.percentiles), arg->valueType());
            },
            arg->valueType());

        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = std::move(aggregate),
            });
    }

    ScalarProjectorPtr planProjector(ir::Projector p) {
        return box<ScalarProjector>(p.alias_field_id, planScalar(std::move(*p.expr)));
    }

    std::vector<ScalarPtr> expressionList(std::vector<ir::Scalar> list) {
        std::vector<ScalarPtr> exprs;
        exprs.reserve(list.size());
        for (auto&& item : list) {
            exprs.push_back(planScalar(std::move(item)));
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

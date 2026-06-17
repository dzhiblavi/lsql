#include "back/exec/plan/plan.h"

#include "back/exec/expr/BinaryScalar.h"
#include "back/exec/expr/CoalesceScalar.h"
#include "back/exec/expr/ConstAggregate.h"
#include "back/exec/expr/CountAllAggregate.h"
#include "back/exec/expr/IdentifierScalar.h"
#include "back/exec/expr/UnaryAggregate.h"
#include "back/exec/expr/UnaryScalar.h"
#include "back/exec/expr/ValueScalar.h"

#include "ir/Aggregates.h"
#include "ir/Relations.h"
#include "ir/Scalars.h"
#include "ir/Statement.h"

#include "util/archive.h"

namespace lsql::back::exec::plan {

namespace {

template <typename P>
FieldSet requiredProjectorsFields(
    const std::vector<Arc<P>>& projectors, const FieldSet& required_projectors) {
    FieldSet set;
    for (auto&& proj : projectors) {
        if (required_projectors.contains(proj->field_id)) {
            set.merge(proj->expr->requiredFields());
        }
    }
    return set;
}

template <typename P>
FieldSet requiredFields(const std::vector<Arc<P>>& projectors) {
    FieldSet set;
    for (auto&& proj : projectors) {
        set.merge(proj->expr->requiredFields());
    }
    return set;
}

FieldSet requiredFields(const std::vector<Arc<Scalar>>& scalars) {
    FieldSet set;
    for (auto&& proj : scalars) {
        set.merge(proj->requiredFields());
    }
    return set;
}

void requestOutput(Arc<Operation> emitter, int phase, FieldSet out_fields);

void init(Aggregate& node, Arc<Operation> op, int /*phase*/) {
    // Aggregate collects its data strictly on its min phase
    requestOutput(node.source, op->min_phase, requiredFields(node.aggregates));
}

void init(Filter& node, Arc<Operation> op, int phase) {
    auto upstream = FieldSet::merge(op->required_fields[phase], node.condition->requiredFields());
    requestOutput(node.source, phase, upstream);
}

void init(Group& node, Arc<Operation> op, int phase) {
    FieldSet upstream;
    upstream.merge(requiredFields(node.group_key));
    upstream.merge(requiredProjectorsFields(node.aggregates, op->required_fields[phase]));
    requestOutput(node.source, phase, upstream);
}

void init(Limit& node, Arc<Operation> op, int phase) {
    auto upstream = op->required_fields[phase];
    requestOutput(node.source, phase, upstream);
}

void init(Log& /*node*/, Arc<Operation> /*op*/, int /*phase*/) {
    // nothing to do
}

void init(MarkJoin& node, Arc<Operation> op, int phase) {
    {  // match
        auto upstream = FieldSet::withField(node.match_field_id);
        requestOutput(node.match, node.match->min_phase, upstream);
    }
    {  // source
        auto upstream = op->required_fields[phase];
        upstream.remove(node.output_field_id);
        upstream.merge(node.scalar->requiredFields());
        requestOutput(node.source, phase, upstream);
    }
}

void init(Materialize& node, Arc<Operation> op, int phase) {
    requestOutput(node.source, op->min_phase, op->required_fields[phase]);
}

void init(MergeSorted& node, Arc<Operation> op, int phase) {
    auto upstream = FieldSet::merge(op->required_fields[phase], requiredFields(node.sort_key));
    requestOutput(node.left, phase, upstream);
    requestOutput(node.right, phase, upstream);
}

void init(Projection& node, Arc<Operation> op, int phase) {
    auto upstream = requiredProjectorsFields(node.projectors, op->required_fields[phase]);
    requestOutput(node.source, phase, upstream);
}

void init(SemiJoin& node, Arc<Operation> op, int phase) {
    {  // match
        auto upstream = FieldSet::withField(node.match_field_id);
        requestOutput(node.match, node.match->min_phase, upstream);
    }
    {  // source
        auto upstream = FieldSet::merge(op->required_fields[phase], node.scalar->requiredFields());
        requestOutput(node.source, phase, upstream);
    }
}

void init(Sort& node, Arc<Operation> op, int phase) {
    auto upstream = FieldSet::merge(op->required_fields[phase], requiredFields(node.sort_key));
    requestOutput(node.source, phase, upstream);
}

void init(TopK& node, Arc<Operation> op, int phase) {
    auto upstream = FieldSet::merge(op->required_fields[phase], requiredFields(node.sort_key));
    requestOutput(node.source, phase, upstream);
}

void init(UnionAll& node, Arc<Operation> op, int phase) {
    auto upstream = op->required_fields[phase];
    requestOutput(node.left, phase, upstream);
    requestOutput(node.right, phase, upstream);
}

void init(Values& /*node*/, Arc<Operation> /*op*/, int /*phase*/) {
    // nothing to do
}

void init(Arc<Operation> op, int phase) {
    util::match(op->node, [&](auto&& node) { init(node, op, phase); });
}

void requestOutput(Arc<Operation> emitter, int phase, FieldSet out_fields) {
    verify(emitter->min_phase <= phase);
    verify(emitter->schema.contains(out_fields));

    emitter->required_fields[phase].merge(out_fields);
    emitter->max_phase = std::max(emitter->max_phase, phase);
    init(emitter, phase);
}

class Planner {
 public:
    explicit Planner(Settings settings) : settings_(std::move(settings)) {}

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

    Arc<Operation> planRelation(ir::Relation s) {
        return util::match(
            std::move(s.node), [&](auto node) { return planRelation(std::move(node), s); });
    }

    ScalarPtr planScalar(ir::Scalar s, const Schema& schema) {
        return util::match(
            std::move(s.node), [&](auto node) { return planScalar(std::move(node), s, schema); });
    }

    Arc<AggregateProjector> planAggregate(ir::Aggregate s, const Schema& schema) {
        return util::match(std::move(s.node), [&](auto node) {
            return planAggregate(std::move(node), s, schema);
        });
    }

    void planStatement(ir::NamedRelationStatement s) {
        auto op = planRelation(std::move(*s.relation));
        named_ops[s.name] = op;
    }

    void planStatement(ir::QueryStatement s) {
        auto rel = planRelation(std::move(*s.relation));
        rel->max_phase = rel->min_phase;
        rel->required_fields[rel->min_phase] = rel->schema.fieldSet();
        init(rel, rel->min_phase);
        plan_.top_operations.push_back(rel);
    }

    Arc<Operation> planRelation(ir::EmptyRelation /*r*/, auto& info) {
        return arc(
            Operation{
                .node = Values{.values = {}},
                .min_phase = 0,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::ValuesRelation r, auto& info) {
        return arc(
            Operation{
                .node = Values{.values = std::move(r.values)},
                .min_phase = 0,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::ProjectionRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto projectors = projectorsList(std::move(r.projectors), source->schema);

        return arc(
            Operation{
                .node =
                    Projection{
                        .source = source,
                        .projectors = projectors,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::AggregateRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto projectors = aggregatesList(std::move(r.aggregates), source->schema);

        return arc(
            Operation{
                .node =
                    Aggregate{
                        .source = source,
                        .aggregates = projectors,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::GroupRelation r, auto& info) {
        auto source_schema = r.source->schema;
        auto source = planRelation(std::move(*r.source));
        auto aggregates = aggregatesList(std::move(r.aggregates), source_schema);
        auto group_key = projectorsList(std::move(r.group_list), source_schema);

        return arc(
            Operation{
                .node =
                    Group{
                        .source = source,
                        .aggregates = aggregates,
                        .group_key = group_key,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::LimitRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));

        return arc(
            Operation{
                .node =
                    Limit{
                        .source = source,
                        .limit = r.limit,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::FilterRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto condition = planScalar(std::move(*r.condition), source->schema);

        return arc(
            Operation{
                .node =
                    Filter{
                        .source = source,
                        .condition = condition,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::SortRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto sort_key = expressionList(std::move(r.order_list), source->schema);

        return arc(
            Operation{
                .node =
                    Sort{
                        .source = source,
                        .sort_key = sort_key,
                        .desc = r.desc,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::TopKRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto sort_key = expressionList(std::move(r.order_list), source->schema);

        return arc(
            Operation{
                .node =
                    TopK{
                        .source = source,
                        .sort_key = sort_key,
                        .desc = r.desc,
                        .top_count = r.top_count,
                    },
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::SemiJoinRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto match = planRelation(std::move(*r.match));
        auto scalar = planScalar(std::move(*r.expr), source->schema);

        return arc(
            Operation{
                .node =
                    SemiJoin{
                        .match = match,
                        .source = source,
                        .scalar = scalar,
                        .match_field_id = r.match_field_id,
                    },
                .min_phase = std::max(match->min_phase + 1, source->min_phase),
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::MarkJoinRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));
        auto match = planRelation(std::move(*r.match));
        auto scalar = planScalar(std::move(*r.expr), source->schema);

        return arc(
            Operation{
                .node =
                    MarkJoin{
                        .match = match,
                        .source = source,
                        .scalar = scalar,
                        .match_field_id = r.match_field_id,
                        .output_field_id = r.output_field_id,
                    },
                .min_phase = std::max(match->min_phase + 1, source->min_phase),
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::UnionAllRelation r, auto& info) {
        auto left = planRelation(std::move(*r.left));
        auto right = planRelation(std::move(*r.right));

        return arc(
            Operation{
                .node =
                    UnionAll{
                        .left = left,
                        .right = right,
                    },
                .min_phase = std::max(left->min_phase, right->min_phase),
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::UnionAllSortedByRelation r, auto& info) {
        auto left = planRelation(std::move(*r.left));
        auto right = planRelation(std::move(*r.right));
        auto sort_key = expressionList(std::move(r.order_list), left->schema);

        return arc(
            Operation{
                .node =
                    MergeSorted{
                        .left = left,
                        .right = right,
                        .sort_key = sort_key,
                        .desc = r.desc,
                    },
                .min_phase = std::max(left->min_phase, right->min_phase),
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::FileRelation r, auto& info) {
        auto time_range =
            util::isProbablyArchive(r.path) ? std::nullopt : settings_.default_time_range;

        return arc(
            Operation{
                .node =
                    Log{
                        .path = r.path,
                        .range = time_range,
                    },
                .min_phase = 0,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::FileIntervalRelation r, auto& info) {
        return arc(
            Operation{
                .node =
                    Log{
                        .path = r.path,
                        .range = TimeRange{r.ts_from, r.ts_to},
                    },
                .min_phase = 0,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::MaterializeRelation r, auto& info) {
        auto source = planRelation(std::move(*r.source));

        return arc(
            Operation{
                .node = Materialize{.source = source},
                .min_phase = source->min_phase,
                .schema = info.schema,
            });
    }

    Arc<Operation> planRelation(ir::NamedRelationReferenceRelation r, auto& /*info*/) {
        require(named_ops.contains(r.name), "no such named relation {}", r.name);
        return named_ops[r.name];
    }

    ScalarPtr planScalar(ir::FieldScalar e, auto& info, auto& schema) {
        auto slot = schema.slot(e.field_id);
        verify(slot.has_value());
        return arc<IdentifierScalar>(*slot, e.field_id, info.value_type);
    }

    ScalarPtr planScalar(ir::ValueScalar e, auto& /*info*/, auto& /*schema*/) {
        return arc<ValueScalar>(e.value);
    }

    ScalarPtr planScalar(ir::CoalesceScalar e, auto& /*info*/, auto& schema) {
        return arc<CoalesceScalar>(expressionList(std::move(e.args), schema));
    }

    ScalarPtr planScalar(ir::CastScalar e, auto& /*info*/, auto& schema) {
        auto arg = planScalar(std::move(*e.expr), schema);
        return arc<UnaryScalar<CastOp>>(arg, arg->valueType(), e.cast_to);
    }

    ScalarPtr planScalar(ir::LikeScalar e, auto& /*info*/, auto& schema) {
        auto arg = planScalar(std::move(*e.expr), schema);
        return arc<UnaryScalar<LikeOp>>(arg, e.regex);
    }

    ScalarPtr planScalar(ir::RSubstrScalar e, auto& /*info*/, auto& schema) {
        auto arg = planScalar(std::move(*e.expr), schema);
        return arc<UnaryScalar<RSubstrOp>>(arg, e.regex);
    }

    ScalarPtr planScalar(ir::UnaryScalar e, auto& /*info*/, auto& schema) {
        auto arg = planScalar(std::move(*e.expr), schema);

        switch (e.type) {
            case UnaryExprType::BooleanNegate:
                return arc<UnaryScalar<BooleanNegationOp>>(arg);
        }
    }

    ScalarPtr planScalar(ir::BinaryScalar e, auto& info, auto& schema) {
        auto l = planScalar(std::move(*e.left), schema);
        auto r = planScalar(std::move(*e.right), schema);

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

    Arc<AggregateProjector> planAggregate(ir::UnaryAggregate a, auto&& info, auto& schema) {
        auto arg = planScalar(std::move(*a.expr), schema);
        auto aggregate = [&] -> AggregatePtr {
            switch (a.type) {
                case UnaryAggregateType::CountNonNull:
                    return arc<UnaryAggregate<CountNonNullOp>>(arg, arg->valueType());
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

    Arc<AggregateProjector> planAggregate(ir::CountAllAggregate, auto&& info, auto& /*schema*/) {
        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = arc<CountAllAggregate>(),
            });
    }

    Arc<AggregateProjector> planAggregate(ir::PercentileAggregate a, auto&& info, auto& schema) {
        auto arg = planScalar(std::move(*a.expr), schema);

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

    Arc<AggregateProjector> planAggregate(ir::ConstAggregate a, auto&& info, auto& /*schema*/) {
        return box(
            AggregateProjector{
                .field_id = info.output_field_id,
                .expr = arc<ConstAggregate>(std::move(a.value), a.null_if_empty),
            });
    }

    Arc<ScalarProjector> planProjector(ir::Projector p, const Schema& schema) {
        return box<ScalarProjector>(p.alias_field_id, planScalar(std::move(*p.expr), schema));
    }

    std::vector<ScalarPtr> expressionList(std::vector<ir::Scalar> list, const Schema& schema) {
        std::vector<ScalarPtr> exprs;
        exprs.reserve(list.size());
        for (auto&& item : list) {
            exprs.push_back(planScalar(std::move(item), schema));
        }
        return exprs;
    }

    std::vector<Arc<ScalarProjector>> projectorsList(
        std::vector<ir::Projector> list, const Schema& schema) {
        std::vector<Arc<ScalarProjector>> proj;
        proj.reserve(list.size());
        for (auto&& item : list) {
            proj.push_back(planProjector(std::move(item), schema));
        }
        return proj;
    }

    std::vector<Arc<AggregateProjector>> aggregatesList(
        std::vector<ir::Aggregate> list, const Schema& schema) {
        std::vector<Arc<AggregateProjector>> proj;
        proj.reserve(list.size());
        for (auto&& item : list) {
            proj.push_back(planAggregate(std::move(item), schema));
        }
        return proj;
    }

    Settings settings_;
    Plan plan_;
    std::unordered_map<std::string, Arc<Operation>> named_ops;
    ConstFieldBindingPtr binding_;
};

}  // namespace

Plan plan(ir::Program program, Settings settings) {
    return Planner(std::move(settings)).plan(std::move(program));
}

}  // namespace lsql::back::exec::plan

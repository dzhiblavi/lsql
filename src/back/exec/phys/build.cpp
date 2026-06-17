#include "back/exec/phys/build.h"

#include "back/exec/phys/log/log.h"
#include "back/exec/phys/ops/Aggregate.h"
#include "back/exec/phys/ops/Filter.h"
#include "back/exec/phys/ops/Group.h"
#include "back/exec/phys/ops/Limit.h"
#include "back/exec/phys/ops/Log.h"
#include "back/exec/phys/ops/MarkJoin.h"
#include "back/exec/phys/ops/Materialize.h"
#include "back/exec/phys/ops/MergeSorted.h"
#include "back/exec/phys/ops/Projection.h"
#include "back/exec/phys/ops/SemiJoin.h"
#include "back/exec/phys/ops/Sort.h"
#include "back/exec/phys/ops/TopK.h"
#include "back/exec/phys/ops/UnionAll.h"
#include "back/exec/phys/ops/Values.h"

namespace lsql::back::exec::phys {

namespace {

template <typename T, typename R = std::decay_t<decltype(std::declval<T>().expr)>>
std::vector<R> projectorsExprs(const std::vector<Arc<T>>& projectors, const FieldSet& required) {
    std::vector<R> exprs;
    exprs.reserve(projectors.size());
    for (auto&& p : projectors) {
        if (required.contains(p->field_id)) {
            exprs.push_back(p->expr);
        } else {
            exprs.emplace_back();
        }
    }
    return exprs;
}

template <typename T, typename R = std::decay_t<decltype(std::declval<T>().expr)>>
std::vector<R> projectorsExprs(const std::vector<Arc<T>>& projectors) {
    std::vector<R> exprs;
    exprs.reserve(projectors.size());
    for (auto&& p : projectors) {
        exprs.push_back(p->expr);
    }
    return exprs;
}

struct Builder {
    explicit Builder(const plan::Plan& plan) : plan(plan) {}

    Program build() {
        for (auto&& op : plan.top_operations) {
            build(*op);

            program.phases[op->min_phase].outputs.push_back({
                .schema = op->schema,
                .operation = get(*op, op->min_phase),
            });
        }

        return std::move(program);
    }

    void build(const plan::Operation& op) {
        if (built.contains(&op)) {
            return;
        }

        util::match(op.node, [&](auto&& node) {
            build(node, op);
            built.insert(&op);
        });
    }

    void build(const plan::Aggregate& aggr, const plan::Operation& op) {
        build(*aggr.source);
        auto state = arc<AggregateState>();
        auto collector = arc<AggregateCollector>(op.id, projectorsExprs(aggr.aggregates), state);
        get(*aggr.source, op.min_phase)->output(collector->sub());
        program.anchors.push_back(collector);

        for (auto&& [phase, _] : op.required_fields) {
            if (phase == op.min_phase) {
                insert(op, phase, collector);
            } else {
                auto emitter = arc<AggregateEmitter>(op.id, state);
                program.phases[phase].sources.push_back(emitter);
                insert(op, phase, emitter);
            }
        }
    }

    void build(const plan::Filter& n, const plan::Operation& op) {
        build(*n.source);

        for (auto&& [phase, _] : op.required_fields) {
            auto opr = arc<Filter>(op.id, n.condition);
            get(*n.source, phase)->output(opr->sub());
            insert(op, phase, opr);
        }
    }

    void build(const plan::Group& n, const plan::Operation& op) {
        build(*n.source);
        auto group_key = projectorsExprs(n.group_key);

        for (auto&& [phase, fields] : op.required_fields) {
            auto aggregates = projectorsExprs(n.aggregates, fields);
            auto opr = arc<Group>(op.id, std::move(aggregates), group_key);

            get(*n.source, phase)->output(opr->sub());
            insert(op, phase, opr);
        }
    }

    void build(const plan::Limit& n, const plan::Operation& op) {
        build(*n.source);

        for (auto&& [phase, _] : op.required_fields) {
            auto opr = arc<Limit>(op.id, n.limit);
            get(*n.source, phase)->output(opr->sub());
            insert(op, phase, opr);
        }
    }

    void build(const plan::Log& n, const plan::Operation& op) {
        auto [lines, type] = open(n);

        for (auto&& [phase, fields] : op.required_fields) {
            absl::flat_hash_map<std::string_view, SlotId> slots;
            uint32_t num_slots = 0;
            for (FieldId id : fields.fieldIds()) {
                auto slot = op.schema.slot(id);
                verify(slot.has_value());
                slots.emplace(plan.field_binding->name(id), *slot);
                num_slots = std::max(num_slots, *slot + 1);
            }

            auto log = arc<Log>(op.id, lines, type, std::move(slots), num_slots);
            insert(op, phase, log);
            program.phases[phase].sources.push_back(log);
        }
    }

    void build(const plan::MarkJoin& n, const plan::Operation& op) {
        build(*n.match);
        build(*n.source);

        auto output_slot = op.schema.slot(n.output_field_id);
        verify(output_slot.has_value());

        auto state = arc<MarkJoinState>();
        auto collector = arc<MarkJoinMatchCollector>(op.id, state);
        get(*n.match, n.match->min_phase)->output(collector->sub());
        program.anchors.push_back(collector);

        for (auto&& [phase, _] : op.required_fields) {
            auto matcher = arc<MarkJoinMatcher>(op.id, state, n.scalar, *output_slot);
            get(*n.source, phase)->output(matcher->sub());
            insert(op, phase, matcher);
        }
    }

    void build(const plan::Materialize& n, const plan::Operation& op) {
        build(*n.source);

        auto state = arc<MaterializeState>();
        auto collector = arc<MaterializeCollector>(op.id, state);
        get(*n.source, op.min_phase)->output(collector->sub());
        program.anchors.push_back(collector);

        for (auto&& [phase, _] : op.required_fields) {
            if (phase == op.min_phase) {
                insert(op, phase, collector);
            } else {
                auto emitter = arc<MaterializeEmitter>(op.id, state);
                program.phases[phase].sources.push_back(emitter);
                insert(op, phase, emitter);
            }
        }
    }

    void build(const plan::MergeSorted& n, const plan::Operation& op) {
        build(*n.left);
        build(*n.right);

        for (auto&& [phase, _] : op.required_fields) {
            auto opr = arc<MergeSorted>(op.id, n.sort_key, n.desc);
            get(*n.left, phase)->output(opr->subLeft());
            get(*n.right, phase)->output(opr->subRight());
            insert(op, phase, opr);
        }
    }

    void build(const plan::Projection& proj, const plan::Operation& op) {
        build(*proj.source);

        for (auto&& [phase, fields] : op.required_fields) {
            auto scalars = projectorsExprs(proj.projectors, fields);
            auto opr = arc<Projection>(op.id, std::move(scalars));

            get(*proj.source, phase)->output(opr->sub());
            insert(op, phase, opr);
        }
    }

    void build(const plan::SemiJoin& n, const plan::Operation& op) {
        build(*n.match);
        build(*n.source);

        auto state = arc<SemiJoinState>();
        auto collector = arc<SemiJoinMatchCollector>(op.id, state);
        get(*n.match, n.match->min_phase)->output(collector->sub());
        program.anchors.push_back(collector);

        for (auto&& [phase, _] : op.required_fields) {
            auto matcher = arc<SemiJoinMatcher>(op.id, state, n.scalar);
            get(*n.source, phase)->output(matcher->sub());
            insert(op, phase, matcher);
        }
    }

    void build(const plan::Sort& n, const plan::Operation& op) {
        build(*n.source);

        for (auto&& [phase, _] : op.required_fields) {
            auto sort = arc<Sort>(op.id, n.sort_key, n.desc);
            get(*n.source, phase)->output(sort->sub());
            insert(op, phase, sort);
        }
    }

    void build(const plan::TopK& n, const plan::Operation& op) {
        build(*n.source);

        for (auto&& [phase, _] : op.required_fields) {
            auto sort = arc<TopK>(op.id, n.sort_key, n.top_count, n.desc);
            get(*n.source, phase)->output(sort->sub());
            insert(op, phase, sort);
        }
    }

    void build(const plan::UnionAll& n, const plan::Operation& op) {
        build(*n.left);
        build(*n.right);

        for (auto&& [phase, _] : op.required_fields) {
            auto un = arc<UnionAll>(op.id);
            get(*n.left, phase)->output(un->subLeft());
            get(*n.right, phase)->output(un->subRight());
            insert(op, phase, un);
        }
    }

    void build(const plan::Values& n, const plan::Operation& op) {
        for (auto&& [phase, _] : op.required_fields) {
            auto values = arc<Values>(op.id, n.values);
            program.phases[phase].sources.push_back(values);
            insert(op, phase, values);
        }
    }

    Arc<Operation> get(const plan::Operation& op, int phase) {
        verify(ops.contains(phase));
        verify(ops[phase].contains(&op));
        return ops[phase][&op];
    }

    void insert(const plan::Operation& op, int phase, Arc<Operation> built) {
        verify(ops[phase][&op] == nullptr);
        ops[phase][&op] = built;
        program.anchors.push_back(built);
    }

    const plan::Plan& plan;
    Program program;

    // phase -> built operations
    std::unordered_set<const plan::Operation*> built;
    std::unordered_map<int, std::unordered_map<const plan::Operation*, Arc<Operation>>> ops;
};

}  // namespace

Program build(const plan::Plan& plan) {
    return Builder(plan).build();
}

}  // namespace lsql::back::exec::phys

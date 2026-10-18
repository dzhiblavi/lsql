#include "optimize/filter_pushdown.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "util/TreeWalker.h"
#include "util/deep_copy.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace lsql::opt {

namespace {

struct FieldRebinder : util::TreeWalker<FieldRebinder> {
    using Base = util::TreeWalker<FieldRebinder>;
    using Base::visit;

    std::unordered_map<FieldId, FieldId> fields;

    void visit(ir::FieldScalar& field) { field.field_id = fields.at(field.field_id); }
};

struct FieldSubsetChecker : util::TreeWalker<FieldSubsetChecker> {
    using Base = util::TreeWalker<FieldSubsetChecker>;
    using Base::visit;

    const Schema& schema;
    bool matches = true;

    void visit(const ir::FieldScalar& field) {
        matches = matches && schema.contains(field.field_id);
    }
};

Box<ir::Scalar> conjunction(Box<ir::Scalar> left, Box<ir::Scalar> right) {
    if (!left) {
        return right;
    }
    if (!right) {
        return left;
    }
    std::vector<ir::Scalar> args;
    args.push_back(std::move(*left));
    args.push_back(std::move(*right));
    return box(
        ir::Scalar{
            .node =
                ir::FnCallScalar{
                    .function = func::And{},
                    .args = std::move(args),
                },
            .value_type = ValueType::Boolean,
        });
}

struct SplitCondition {
    Box<ir::Scalar> left;
    Box<ir::Scalar> remaining;
};

SplitCondition splitLeftCondition(ir::Scalar condition, const Schema& left_schema) {
    auto* call = std::get_if<ir::FnCallScalar>(&condition.node);
    if (call && std::holds_alternative<func::And>(call->function)) {
        auto left = splitLeftCondition(std::move(call->args[0]), left_schema);
        auto right = splitLeftCondition(std::move(call->args[1]), left_schema);
        return {
            .left = conjunction(std::move(left.left), std::move(right.left)),
            .remaining = conjunction(std::move(left.remaining), std::move(right.remaining)),
        };
    }

    FieldSubsetChecker checker{.schema = left_schema};
    checker.visit(std::as_const(condition));
    if (checker.matches) {
        return {.left = box(std::move(condition)), .remaining = nullptr};
    }
    return {.left = nullptr, .remaining = box(std::move(condition))};
}

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void addFilter(ir::Relation& source, ir::Scalar condition) {
        auto schema = source.schema;
        auto input = box(std::move(source));
        source = ir::Relation{
            .node =
                ir::FilterRelation{
                    .source = std::move(input),
                    .condition = box(std::move(condition)),
                },
            .schema = std::move(schema),
        };
        pushdown(source);
    }

    void removeFilter(ir::FilterRelation& filter, ir::Relation& self) {
        ctx.setChanges();
        auto source = std::move(*filter.source);
        self = std::move(source);
    }

    void pushdown(ir::FilterRelation& filter, ir::UnionAllRelation& union_all, ir::Relation& self) {
        for (auto& source : union_all.sources) {
            addFilter(source, util::deepCopy(*filter.condition));
        }
        ctx.note("filter pushed into union inputs");
        removeFilter(filter, self);
    }

    void pushdown(
        ir::FilterRelation& filter, ir::UnionAllSortedByRelation& merge, ir::Relation& self) {
        addFilter(*merge.left, util::deepCopy(*filter.condition));
        addFilter(*merge.right, util::deepCopy(*filter.condition));
        ctx.note("filter pushed into sorted merge inputs");
        removeFilter(filter, self);
    }

    void pushdown(
        ir::FilterRelation& filter, ir::ProjectionRelation& projection, ir::Relation& self) {
        FieldRebinder rebinder;
        for (const auto& projector : projection.projectors) {
            auto* field = std::get_if<ir::FieldScalar>(&projector.expr->node);
            if (!field) {
                return;
            }
            rebinder.fields.emplace(projector.alias_field_id, field->field_id);
        }

        rebinder.visit(*filter.condition);
        addFilter(*projection.source, std::move(*filter.condition));
        ctx.note("filter pushed through field projection");
        removeFilter(filter, self);
    }

    void pushdown(ir::FilterRelation& filter, ir::LeftJoinRelation& join, ir::Relation& self) {
        auto split = splitLeftCondition(util::deepCopy(*filter.condition), join.left->schema);
        if (!split.left) {
            return;
        }

        addFilter(*join.left, std::move(*split.left));
        ctx.note("filter pushed into left join left input");
        if (split.remaining) {
            filter.condition = std::move(split.remaining);
            ctx.setChanges();
            return;
        }
        removeFilter(filter, self);
    }

    void pushdown(ir::FilterRelation& filter, ir::SortRelation& sort, ir::Relation& self) {
        addFilter(*sort.source, std::move(*filter.condition));
        ctx.note("filter pushed through sort");
        removeFilter(filter, self);
    }

    void pushdown(ir::Relation& self) {
        auto* filter = std::get_if<ir::FilterRelation>(&self.node);
        if (!filter) {
            return;
        }

        if (auto* union_all = std::get_if<ir::UnionAllRelation>(&filter->source->node)) {
            pushdown(*filter, *union_all, self);
        } else if (auto* merge = std::get_if<ir::UnionAllSortedByRelation>(&filter->source->node)) {
            pushdown(*filter, *merge, self);
        } else if (auto* projection = std::get_if<ir::ProjectionRelation>(&filter->source->node)) {
            pushdown(*filter, *projection, self);
        } else if (auto* join = std::get_if<ir::LeftJoinRelation>(&filter->source->node)) {
            pushdown(*filter, *join, self);
        } else if (auto* sort = std::get_if<ir::SortRelation>(&filter->source->node)) {
            pushdown(*filter, *sort, self);
        }
    }

    void visit(ir::Relation& self) {
        Base::visit(self);
        pushdown(self);
    }
};

}  // namespace

ir::Program filterPushdown(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

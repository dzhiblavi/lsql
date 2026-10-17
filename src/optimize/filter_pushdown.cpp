#include "optimize/filter_pushdown.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "util/TreeWalker.h"

#include <unordered_map>

namespace lsql::opt {

namespace {

struct FieldRebinder : util::TreeWalker<FieldRebinder> {
    using Base = util::TreeWalker<FieldRebinder>;
    using Base::visit;

    std::unordered_map<FieldId, FieldId> fields;

    void visit(ir::FieldScalar& field) { field.field_id = fields.at(field.field_id); }
};

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

    void pushdown(ir::Relation& self) {
        auto* filter = std::get_if<ir::FilterRelation>(&self.node);
        if (!filter) {
            return;
        }

        if (auto* union_all = std::get_if<ir::UnionAllRelation>(&filter->source->node)) {
            for (auto& source : union_all->sources) {
                addFilter(source, *filter->condition);
            }
            ctx.note("filter pushed into union inputs");
        } else if (auto* merge = std::get_if<ir::UnionAllSortedByRelation>(&filter->source->node)) {
            addFilter(*merge->left, *filter->condition);
            addFilter(*merge->right, *filter->condition);
            ctx.note("filter pushed into sorted merge inputs");
        } else if (auto* projection = std::get_if<ir::ProjectionRelation>(&filter->source->node)) {
            FieldRebinder rebinder;
            for (const auto& projector : projection->projectors) {
                auto* field = std::get_if<ir::FieldScalar>(&projector.expr->node);
                if (!field) {
                    return;
                }
                rebinder.fields.emplace(projector.alias_field_id, field->field_id);
            }

            rebinder.visit(*filter->condition);
            addFilter(*projection->source, std::move(*filter->condition));
            ctx.note("filter pushed through field projection");
        } else {
            return;
        }

        ctx.setChanges();
        auto source = std::move(*filter->source);
        self = std::move(source);
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

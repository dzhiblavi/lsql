#include "optimize/filter_pushdown.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "util/TreeWalker.h"

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void addFilter(ir::Relation& source, const ir::Scalar& condition) {
        auto schema = source.schema;
        auto input = box(std::move(source));
        source = ir::Relation{
            .node =
                ir::FilterRelation{
                    .source = std::move(input),
                    .condition = box(condition),
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
        } else if (auto* merge = std::get_if<ir::UnionAllSortedByRelation>(&filter->source->node)) {
            addFilter(*merge->left, *filter->condition);
            addFilter(*merge->right, *filter->condition);
        } else {
            return;
        }

        ctx.setChanges().note("filter pushed into union inputs");
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

#include "optimize/relation_simplify.h"
#include "optimize/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ConsumePass<Optimizer> {
    Context& ctx;

    bool isForwardingProjection(const ir::ProjectionRelation& p) {
        for (auto&& proj : p.projectors) {
            auto* field_expr = std::get_if<ir::FieldScalar>(&proj.expr->node);
            if (field_expr == nullptr) {
                // not a field expression: not our case
                return false;
            }

            if (field_expr->field_id != proj.alias_field_id) {
                // field expression does some renaming apparently
                return false;
            }
        }

        // All projectors are identity projectors
        return true;
    }

    ir::Relation optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (isForwardingProjection(rel)) {
            ctx.setChanges().note("forwarding projection collapsed");
            return std::move(*rel.source);
        }

        return std::move(self);
    }

    ir::Relation optimize(ir::FilterRelation& rel, ir::Relation& self) {
        auto* v = std::get_if<ir::ValueScalar>(&rel.condition->node);
        if (!v) {
            return std::move(self);
        }

        ctx.setChanges().note("filter condition collapsed empty={}", v->value.get<bool>());

        if (v->value == true) {
            return std::move(*rel.source);
        }

        self.node = ir::EmptyRelation{};
        return std::move(self);
    }

    ir::Relation optimize(ir::LimitRelation& rel, ir::Relation& self) {
        // Limit(Sort(X), L) => TopK(X, L)
        auto* sort = std::get_if<ir::SortRelation>(&rel.source->node);
        if (sort) {
            ctx.setChanges().note("sort/limit -> topk");

            self.node = ir::TopKRelation{
                .source = std::move(sort->source),
                .order_list = std::move(sort->order_list),
                .desc = sort->desc,
                .top_count = rel.limit,
            };
            return std::move(self);
        }

        // Limit(Projection(X), L) => Projection(Limit(X, L))
        auto* projection = std::get_if<ir::ProjectionRelation>(&rel.source->node);
        if (projection) {
            ctx.setChanges().note("limit(projection) -> projection(limit)");
            auto proj_source_fields = projection->source->fields_out;

            self.node = ir::ProjectionRelation{
                .source =
                    box(ir::Relation{
                        .node =
                            ir::LimitRelation{
                                .source = std::move(projection->source),
                                .limit = rel.limit,
                            },
                        .fields_out = proj_source_fields,
                    }),
                .projectors = std::move(projection->projectors),
            };
            return std::move(self);
        }

        // Limit(Limit(X, A), B) => Limit(X, min(A, B))
        auto* limit = std::get_if<ir::LimitRelation>(&rel.source->node);
        if (limit) {
            ctx.setChanges().note("limit(limit) -> limit");

            self.node = ir::LimitRelation{
                .source = std::move(limit->source),
                .limit = std::min(rel.limit, limit->limit),
            };
            return std::move(self);
        }

        // Limit(TopK(X, A), B) => TopK(X, min(A, B))
        auto* topk = std::get_if<ir::TopKRelation>(&rel.source->node);
        if (topk) {
            ctx.setChanges().note("limit(topk) -> topk");

            self.node = ir::TopKRelation{
                .source = std::move(topk->source),
                .order_list = std::move(topk->order_list),
                .desc = topk->desc,
                .top_count = std::min(rel.limit, topk->top_count),
            };
            return std::move(self);
        }

        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
        auto pass_name = "relation_simplify";
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { optimize(node, self); }) {
            llog::trace("applying {} step for {}", pass_name, name);
            return optimize(node, self);
        } else {
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program relationSimplify(ir::Program program, Context& ctx) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    Optimizer opt{.ctx = ctx};
    for (auto&& statement : program.statements) {
        result.statements.push_back(opt.pass(std::move(statement)));
    }

    return result;
}

}  // namespace lsql::opt

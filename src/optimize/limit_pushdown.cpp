#include "optimize/limit_pushdown.h"
#include "ir/pass.h"

#include <llog/log.h>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    void ensureLimited(ir::LimitRelation& lim, Box<ir::Relation>& node, auto label, auto name) {
        if (auto* limit = std::get_if<ir::LimitRelation>(&node->node)) {
            if (limit->limit > lim.limit) {
                ctx.setChanges().note("limit({}(L, R), N): re-limit {} by N", label, name);
                limit->limit = lim.limit;
            }
            return;
        }

        if (auto* topk = std::get_if<ir::TopKRelation>(&node->node)) {
            if (topk->top_count > lim.limit) {
                ctx.setChanges().note("limit({}(L, R), N): re-topk {} by N", label, name);
                topk->top_count = lim.limit;
            }
            return;
        }

        ctx.setChanges().note("limit({}(L, R), N): limit {} by N", label, name);
        auto schema = node->schema;
        node =
            box(ir::Relation{
                .node = ir::LimitRelation{.source = std::move(node), .limit = lim.limit},
                .schema = schema,
            });
    }

    ir::Relation pushdown(auto& lim, ir::ProjectionRelation& src, auto& self) {
        ctx.setChanges().note("limit(projection) -> projection(limit)");
        auto proj_source_fields = src.source->schema;
        self.node = ir::ProjectionRelation{
            .source =
                box(ir::Relation{
                    .node =
                        ir::LimitRelation{
                            .source = std::move(src.source),
                            .limit = lim.limit,
                        },
                    .schema = proj_source_fields,
                }),
            .projectors = std::move(src.projectors),
        };
        return std::move(self);
    }

    ir::Relation pushdown(auto& lim, ir::SortRelation& src, auto& self) {
        ctx.setChanges().note("sort/limit -> topk");
        self.node = ir::TopKRelation{
            .source = std::move(src.source),
            .order_list = std::move(src.order_list),
            .desc = src.desc,
            .top_count = lim.limit,
        };
        return std::move(self);
    }

    ir::Relation pushdown(auto& lim, ir::LimitRelation& src, auto& self) {
        ctx.setChanges().note("limit(limit) -> limit");
        self.node = ir::LimitRelation{
            .source = std::move(src.source),
            .limit = std::min(lim.limit, src.limit),
        };
        return std::move(self);
    }

    ir::Relation pushdown(auto& lim, ir::TopKRelation& src, auto& self) {
        ctx.setChanges().note("limit(topk) -> topk");
        self.node = ir::TopKRelation{
            .source = std::move(src.source),
            .order_list = std::move(src.order_list),
            .desc = src.desc,
            .top_count = std::min(lim.limit, src.top_count),
        };
        return std::move(self);
    }

    ir::Relation pushdown(auto& lim, ir::UnionAllRelation& src, auto& self) {
        ensureLimited(lim, src.left, "union_all", "L");
        ensureLimited(lim, src.right, "union_all", "R");
        return std::move(self);
    }

    ir::Relation pushdown(auto& lim, ir::UnionAllSortedByRelation& src, auto& self) {
        ensureLimited(lim, src.left, "union_all_sorted_by", "L");
        ensureLimited(lim, src.right, "union_all_sorted_by", "R");
        return std::move(self);
    }

    ir::Relation construct(ir::LimitRelation& rel, ir::Relation& self) {
        return util::match(rel.source->node, [&](auto& src) {
            if constexpr (requires { pushdown(rel, src, self); }) {
                llog::trace("applying limit_pushdown step for ir::LimitRelation");
                return pushdown(rel, src, self);
            }
            return std::move(self);
        });
    }

    auto construct(auto& /*node*/, auto& self) { return std::move(self); }
};

}  // namespace

ir::Program limitPushdown(ir::Program program, Context& ctx) {
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

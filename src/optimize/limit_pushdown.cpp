#include "optimize/limit_pushdown.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"  // IWYU pragma: keep

#include "util/TreeWalker.h"
#include "util/logging.h"

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    void visit(ir::Relation& rel) {
        Base::visit(rel);

        if (auto* limit = std::get_if<ir::LimitRelation>(&rel.node)) {
            util::match(limit->source->node, [&](auto& src) {
                if constexpr (requires { pushdown(*limit, src, rel); }) {
                    llog::trace("applying limit_pushdown step for ir::LimitRelation");
                    pushdown(*limit, src, rel);
                }
            });
        }
    }

    void ensureLimited(ir::LimitRelation& lim, ir::Relation& node, auto label, auto name) {
        if (auto* limit = std::get_if<ir::LimitRelation>(&node.node)) {
            if (limit->limit > lim.limit) {
                ctx.setChanges().note("limit({}(L, R), N): re-limit {} by N", label, name);
                limit->limit = lim.limit;
            }
            return;
        }

        if (auto* topk = std::get_if<ir::TopKRelation>(&node.node)) {
            if (topk->top_count > lim.limit) {
                ctx.setChanges().note("limit({}(L, R), N): re-topk {} by N", label, name);
                topk->top_count = lim.limit;
            }
            return;
        }

        ctx.setChanges().note("limit({}(L, R), N): limit {} by N", label, name);
        auto schema = node.schema;
        node = ir::Relation{
            .node = ir::LimitRelation{.source = box(std::move(node)), .limit = lim.limit},
            .schema = schema,
        };
    }

    void pushdown(auto& lim, ir::ProjectionRelation& src, auto& self) {
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
    }

    void pushdown(auto& lim, ir::SortRelation& src, auto& self) {
        ctx.setChanges().note("sort/limit -> topk");
        self.node = ir::TopKRelation{
            .source = std::move(src.source),
            .order_list = std::move(src.order_list),
            .desc = src.desc,
            .top_count = lim.limit,
        };
    }

    void pushdown(auto& lim, ir::LimitRelation& src, auto& self) {
        ctx.setChanges().note("limit(limit) -> limit");
        self.node = ir::LimitRelation{
            .source = std::move(src.source),
            .limit = std::min(lim.limit, src.limit),
        };
    }

    void pushdown(auto& lim, ir::TopKRelation& src, auto& self) {
        ctx.setChanges().note("limit(topk) -> topk");
        self.node = ir::TopKRelation{
            .source = std::move(src.source),
            .order_list = std::move(src.order_list),
            .desc = src.desc,
            .top_count = std::min(lim.limit, src.top_count),
        };
    }

    void pushdown(auto& lim, ir::UnionAllRelation& u, auto& /*self*/) {
        for (auto&& src : u.sources) {
            ensureLimited(lim, src, "union_all", "C");
        }
    }

    void pushdown(auto& lim, ir::UnionAllSortedByRelation& src, auto& /*self*/) {
        ensureLimited(lim, *src.left, "union_all_sorted_by", "L");
        ensureLimited(lim, *src.right, "union_all_sorted_by", "R");
    }

    Context& ctx;
};

}  // namespace

ir::Program limitPushdown(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};

    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

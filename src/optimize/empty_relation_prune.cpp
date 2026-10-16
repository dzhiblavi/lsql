#include "optimize/empty_relation_prune.h"
#include "ir/Aggregates.h"
#include "ir/Relations.h"
#include "ir/Scalars.h"
#include "util/TreeWalker.h"
#include "util/logging.h"

#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    bool isEmpty(const ir::Relation& r) {
        return std::holds_alternative<ir::EmptyRelation>(r.node);
    }

    void pruneSimple(auto& rel, auto& self) {
        if (!isEmpty(*rel.source)) {
            return;
        }

        ctx.setChanges().note("empty-sourced relation pruned");
        self.node = ir::EmptyRelation{};
    }

    void prune(ir::ProjectionRelation& rel, auto& self) { pruneSimple(rel, self); }
    void prune(ir::GroupRelation& rel, auto& self) { pruneSimple(rel, self); }
    void prune(ir::FilterRelation& rel, auto& self) { pruneSimple(rel, self); }
    void prune(ir::SortRelation& rel, auto& self) { pruneSimple(rel, self); }
    void prune(ir::MaterializeRelation& rel, auto& self) { pruneSimple(rel, self); }

    void prune(ir::UnionAllRelation& rel, auto& self) {
        std::vector<ir::Relation> non_empty;

        for (auto&& src : rel.sources) {
            if (!isEmpty(src)) {
                non_empty.push_back(std::move(src));
            }
        }

        if (non_empty.empty()) {
            ctx.setChanges().note("all union-all sources are empty");
            self.node = ir::EmptyRelation{};
            return;
        }

        if (non_empty.size() == 1) {
            ctx.setChanges().note("only one union-all source is not empty");
            self = std::move(non_empty[0]);
            return;
        }

        if (non_empty.size() != rel.sources.size()) {
            ctx.setChanges().note("some of union-all sources are empty");
        }

        rel.sources = std::move(non_empty);
    }

    void prune(ir::UnionAllSortedByRelation& rel, auto& self) {
        if (isEmpty(*rel.left) && isEmpty(*rel.right)) {
            ctx.setChanges().note("empty union-all sources pruned");
            self.node = ir::EmptyRelation{};
            return;
        }

        if (isEmpty(*rel.left)) {
            ctx.setChanges().note("empty union-all left source pruned");
            auto source = std::move(*rel.right);
            self = std::move(source);
            return;
        }

        if (isEmpty(*rel.right)) {
            ctx.setChanges().note("empty union-all right source pruned");
            auto source = std::move(*rel.left);
            self = std::move(source);
        }
    }

    void prune(ir::TopKRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || rel.top_count == 0) {
            ctx.setChanges().note("empty topk relation pruned");
            self.node = ir::EmptyRelation{};
        }
    }

    void prune(ir::LimitRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || rel.limit == 0) {
            ctx.setChanges().note("empty limit relation pruned");
            self.node = ir::EmptyRelation{};
        }
    }

    void prune(ir::SemiJoinRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || isEmpty(*rel.match)) {
            ctx.setChanges().note("empty semi-join relation pruned");
            self.node = ir::EmptyRelation{};
        }
    }

    void prune(ir::LeftJoinRelation& rel, auto& self) {
        if (isEmpty(*rel.left)) {
            ctx.setChanges().note("empty left-join source pruned");
            self.node = ir::EmptyRelation{};
        }
    }

    void prune(ir::MarkJoinRelation& rel, auto& self) {
        if (isEmpty(*rel.source)) {
            ctx.setChanges().note("empty mark-join source pruned");
            self.node = ir::EmptyRelation{};
        }
    }

    void visit(ir::Relation& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { prune(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying empty_relation_prune step for {}", name);
                prune(node, self);
            }
        });
    }
};

}  // namespace

ir::Program emptyRelationPrune(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

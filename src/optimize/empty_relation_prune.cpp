#include "optimize/empty_relation_prune.h"
#include "ir/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    bool isEmpty(const ir::Relation& r) {
        return std::holds_alternative<ir::EmptyRelation>(r.node);
    }

    ir::Relation pruneSimple(auto& rel, auto& self) {
        if (!isEmpty(*rel.source)) {
            return std::move(self);
        }

        ctx.setChanges().note("empty-sourced relation pruned");
        self.node = ir::EmptyRelation{};
        return std::move(self);
    }

    ir::Relation pruneUnion(auto& rel, auto& self) {
        if (isEmpty(*rel.left) && isEmpty(*rel.right)) {
            ctx.setChanges().note("empty union-all sources pruned");
            self.node = ir::EmptyRelation{};
            return std::move(self);
        }

        if (isEmpty(*rel.left)) {
            ctx.setChanges().note("empty union-all left source pruned");
            return std::move(*rel.right);
        }

        if (isEmpty(*rel.right)) {
            ctx.setChanges().note("empty union-all right source pruned");
            return std::move(*rel.left);
        }

        return std::move(self);
    }

    ir::Relation prune(ir::ProjectionRelation& rel, auto& self) { return pruneSimple(rel, self); }
    ir::Relation prune(ir::AggregateRelation& /*rel*/, auto& self) { return std::move(self); }
    ir::Relation prune(ir::GroupRelation& rel, auto& self) { return pruneSimple(rel, self); }
    ir::Relation prune(ir::FilterRelation& rel, auto& self) { return pruneSimple(rel, self); }
    ir::Relation prune(ir::SortRelation& rel, auto& self) { return pruneSimple(rel, self); }
    ir::Relation prune(ir::MaterializeRelation& rel, auto& self) { return pruneSimple(rel, self); }

    ir::Relation prune(ir::UnionAllRelation& rel, auto& self) { return pruneUnion(rel, self); }

    ir::Relation prune(ir::UnionAllSortedByRelation& rel, auto& self) {
        return pruneUnion(rel, self);
    }

    ir::Relation prune(ir::TopKRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || rel.top_count == 0) {
            ctx.setChanges().note("empty topk relation pruned");
            self.node = ir::EmptyRelation{};
        }
        return std::move(self);
    }

    ir::Relation prune(ir::LimitRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || rel.limit == 0) {
            ctx.setChanges().note("empty limit relation pruned");
            self.node = ir::EmptyRelation{};
        }
        return std::move(self);
    }

    ir::Relation prune(ir::SemiJoinRelation& rel, auto& self) {
        if (isEmpty(*rel.source) || isEmpty(*rel.match)) {
            ctx.setChanges().note("empty semi-join relation pruned");
            self.node = ir::EmptyRelation{};
        }
        return std::move(self);
    }

    ir::Relation prune(ir::MarkJoinRelation& rel, auto& self) {
        if (isEmpty(*rel.source)) {
            ctx.setChanges().note("empty mark-join source pruned");
            self.node = ir::EmptyRelation{};
        }
        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
        auto pass_name = "empty_relation_prune";
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { prune(node, self); }) {
            llog::trace("applying {} step for {}", pass_name, name);
            return prune(node, self);
        } else {
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program emptyRelationPrune(ir::Program program, Context& ctx) {
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

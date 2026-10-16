#include "optimize/relation_simplify.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
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

    bool isForwardingProjection(const ir::ProjectionRelation& p, auto& self) {
        if (self.schema != p.source->schema) {
            return false;
        }

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

    void optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (isForwardingProjection(rel, self)) {
            ctx.setChanges().note("forwarding projection collapsed");
            auto source = std::move(*rel.source);
            self = std::move(source);
        }
    }

    void optimize(ir::FilterRelation& rel, ir::Relation& self) {
        auto* v = std::get_if<ir::ValueScalar>(&rel.condition->node);
        if (!v) {
            return;
        }

        ctx.setChanges().note("filter condition collapsed empty={}", v->value.get<bool>());

        if (v->value == true) {
            auto source = std::move(*rel.source);
            self = std::move(source);
            return;
        }

        self.node = ir::EmptyRelation{};
    }

    void optimize(ir::UnionAllRelation& rel, ir::Relation&) {
        std::vector<ir::Relation> sources;
        for (auto&& source : rel.sources) {
            if (auto* nested = std::get_if<ir::UnionAllRelation>(&source.node)) {
                ctx.setChanges().note("nested union-all collapsed");
                for (auto&& nested_source : nested->sources) {
                    sources.push_back(std::move(nested_source));
                }
            } else {
                sources.push_back(std::move(source));
            }
        }

        rel.sources = std::move(sources);
    }

    void visit(ir::Relation& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { optimize(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying relation_simplify step for {}", name);
                optimize(node, self);
            }
        });
    }
};

}  // namespace

ir::Program relationSimplify(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

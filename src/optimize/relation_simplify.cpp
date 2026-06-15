#include "optimize/relation_simplify.h"
#include "ir/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
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

    ir::Relation optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (isForwardingProjection(rel, self)) {
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

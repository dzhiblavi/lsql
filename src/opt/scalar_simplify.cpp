#include "opt/scalar_simplify.h"
#include "opt/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ConsumePass<Optimizer> {
    ir::Scalar optimize(ir::CoalesceScalar& s, auto& self) {
        auto args = std::move(s.args);
        s.args.reserve(args.size());

        for (auto&& a : args) {
            if (auto* v = std::get_if<ir::ValueScalar>(&a.node)) {
                if (v->value == null) {
                    // just skip nulls, they don't make a difference
                    continue;
                }

                if (s.args.empty()) {
                    // first value collapsed => done
                    self.node = ir::ValueScalar{.value = std::move(v->value)};
                    return std::move(self);
                }
            }

            // not a constant
            s.args.push_back(std::move(a));
        }

        if (s.args.empty()) {
            self.node = ir::ValueScalar{.value = null};
        }

        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
        auto pass_name = "scalar_simplify";
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

ir::Program scalarSimplify(ir::Program program) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    Optimizer opt;
    for (auto&& statement : program.statements) {
        result.statements.push_back(opt.pass(std::move(statement)));
    }

    return result;
}

}  // namespace lsql::opt

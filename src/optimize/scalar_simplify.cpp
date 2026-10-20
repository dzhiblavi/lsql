#include "optimize/scalar_simplify.h"
#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"
#include "util/TreeWalker.h"
#include "util/logging.h"

#include <rfl.hpp>

#include <algorithm>

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void simplifyBooleanBinary(ir::FnCallScalar& call, ir::Scalar& self, bool conjunction) {
        verify_dbg(call.args.size() == 2);

        for (size_t index = 0; index < call.args.size(); ++index) {
            auto* constant = std::get_if<ir::ValueScalar>(&call.args[index].node);
            auto& other = call.args[1 - index];
            if (!constant || constant->value.type() != ValueType::Boolean) {
                continue;
            }

            ctx.setChanges().note("boolean constant operand simplified");
            auto replacement =
                std::move(constant->value.get<bool>() == conjunction ? other : call.args[index]);
            self = std::move(replacement);
            return;
        }
    }

    void simplifyDoubleNegation(ir::FnCallScalar& call, ir::Scalar& self) {
        verify_dbg(call.args.size() == 1);

        auto* inner = std::get_if<ir::FnCallScalar>(&call.args[0].node);
        if (!inner || !std::holds_alternative<func::BooleanNegate>(inner->function)) {
            return;
        }

        ctx.setChanges().note("double boolean negation removed");
        auto replacement = std::move(inner->args[0]);
        self = std::move(replacement);
    }

    void optimize(ir::FnCallScalar& s, auto& self) {
        if (auto* cast = std::get_if<func::Cast>(&s.function)) {
            verify_dbg(s.args.size() == 1);
            if (s.args[0].value_type == cast->cast_to) {
                ctx.setChanges().note("identity cast removed");
                auto scalar = std::move(s.args[0]);
                self = std::move(scalar);
            }
            return;
        }

        if (std::holds_alternative<func::And>(s.function)) {
            simplifyBooleanBinary(s, self, true);
            return;
        }

        if (std::holds_alternative<func::Or>(s.function)) {
            simplifyBooleanBinary(s, self, false);
            return;
        }

        if (std::holds_alternative<func::BooleanNegate>(s.function)) {
            simplifyDoubleNegation(s, self);
            return;
        }

        if (std::holds_alternative<func::Coalesce>(s.function)) {
            // turn coalesce function into a more efficient IR node
            ctx.setChanges().note("coalesce func -> coalesce node");
            self.node = ir::CoalesceScalar{.args = std::move(s.args)};
        }
    }

    void optimize(ir::CoalesceScalar& s, auto& self) {
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
                    ctx.setChanges().note("coalesce folded to a constant");
                    self.node = ir::ValueScalar{.value = std::move(v->value)};
                    return;
                }

                s.args.push_back(std::move(a));
                break;
            }

            if (std::ranges::any_of(s.args, [&](const auto& previous) { return previous == a; })) {
                continue;
            }

            // not a constant
            s.args.push_back(std::move(a));
        }

        if (s.args.empty()) {
            ctx.setChanges().note("coalesce folded to a null");
            self.node = ir::ValueScalar{.value = null};
            return;
        }

        if (s.args.size() == 1) {
            ctx.setChanges().note("coalesce folded to a single scalar");
            auto scalar = std::move(s.args[0]);
            self = std::move(scalar);
            return;
        }

        if (s.args.size() != args.size()) {
            ctx.setChanges().note(
                "some coalesce folded: was {}, now {}", args.size(), s.args.size());
        }
    }

    void visit(ir::Scalar& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { optimize(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying scalar_simplify step for {}", name);
                optimize(node, self);
            }
        });
    }
};

}  // namespace

ir::Program scalarSimplify(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

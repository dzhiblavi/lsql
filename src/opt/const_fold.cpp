#include "opt/const_fold.h"
#include "opt/pass.h"

#include "core/valueCast.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer {
    ir::Scalar optimize(ir::CastScalar& s, auto& self) {
        if (auto* v = std::get_if<ir::ValueScalar>(&s.expr->node)) {
            self.node = ir::ValueScalar{.value = valueCast(std::move(v->value), s.cast_to)};
        }
        return std::move(self);
    }

    ir::Scalar optimize(ir::UnaryScalar& s, auto& self) {
        auto* v = std::get_if<ir::ValueScalar>(&s.expr->node);
        if (!v) {
            return std::move(self);
        }

        auto value = [&] -> Value {
            switch (s.type) {
                case UnaryExprType::BooleanNegate:
                    return !v->value.get<bool>();
            }
        }();

        self.node = ir::ValueScalar{.value = value};
        return std::move(self);
    }

    ir::Scalar optimize(ir::BinaryScalar& s, auto& self) {
        auto* vl = std::get_if<ir::ValueScalar>(&s.left->node);
        auto* vr = std::get_if<ir::ValueScalar>(&s.right->node);
        if (!vl || !vr) {
            return std::move(self);
        }

        auto value = [&] -> Value {
            switch (s.type) {
                case BinaryExprType::Equal:
                    return vl->value == vr->value;
                case BinaryExprType::NotEqual:
                    return vl->value != vr->value;
                case BinaryExprType::And:
                    return vl->value.get<bool>() && vr->value.get<bool>();
                case BinaryExprType::Or:
                    return vl->value.get<bool>() || vr->value.get<bool>();
                case BinaryExprType::Divide:
                    return std::visit(
                        util::Overloaded{
                            []<Dividable T>(T& l, T& r) -> Value { return r == 0 ? 0 : l / r; },
                            [](auto&&...) -> Value { panic(); },
                        },
                        vl->value.variant(),
                        vr->value.variant());
                case BinaryExprType::Add:
                    return std::visit(
                        util::Overloaded{
                            []<Addable T>(T& l, T& r) -> Value { return l + r; },
                            [](auto&&...) -> Value { panic(); },
                        },
                        vl->value.variant(),
                        vr->value.variant());

                case BinaryExprType::Subtract:
                    return std::visit(
                        util::Overloaded{
                            []<Subtractable T>(T& l, T& r) -> Value { return l - r; },
                            [](auto&&...) -> Value { panic(); },
                        },
                        vl->value.variant(),
                        vr->value.variant());
            }
        }();

        self.node = ir::ValueScalar{.value = value};
        return std::move(self);
    }

    auto operator()(auto& node, auto& self) {
        auto step_name = "const_fold";
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { optimize(node, self); }) {
            llog::trace("applying {} step for {}", step_name, name);
            return optimize(node, self);
        } else {
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program constFold(ir::Program program) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    Optimizer opt;
    for (auto&& statement : program.statements) {
        result.statements.push_back(pass(std::move(statement), opt));
    }

    return result;
}

}  // namespace lsql::opt

#include "optimize/const_fold.h"
#include "ir/pass.h"

#include "core/valueCast.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    ir::Scalar optimize(ir::CastScalar& s, auto& self) {
        auto* v = std::get_if<ir::ValueScalar>(&s.expr->node);
        if (!v) {
            return std::move(self);
        }

        ctx.setChanges().note("CastScalar const propagation");
        self.node = ir::ValueScalar{.value = valueCast(std::move(v->value), s.cast_to)};
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

        ctx.setChanges().note("UnaryScalar const propagation");
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

        ctx.setChanges().note("BinaryScalar const propagation");
        self.node = ir::ValueScalar{.value = value};
        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
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

ir::Program constFold(ir::Program program, Context& ctx) {
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

#include "optimize/const_scalar_fold.h"

#include "ir/pass.h"

#include "core/function/build.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    ir::Scalar optimize(ir::FnCallScalar& s, auto& self) {
        if (!func::isScalar(s.function)) {
            return std::move(self);
        }

        bool all_const = std::ranges::all_of(s.args, [](ir::Scalar& arg) {
            return std::holds_alternative<ir::ValueScalar>(arg.node);
        });

        if (!all_const) {
            return std::move(self);
        }

        std::vector<Value> values;
        values.reserve(s.args.size());
        for (auto&& arg : s.args) {
            values.push_back(std::get<ir::ValueScalar>(arg.node).value);
        }

        return func::buildScalar<ir::Scalar>(s.function, [&]<func::Executor E>(E executor) {
            ctx.setChanges().note("folded constant scalar function call");
            auto result = executor.execute(values);

            return ir::Scalar{
                .node = ir::ValueScalar{.value = std::move(result)},
                .value_type = self.value_type,
            };
        });
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
                            []<Dividable T>(T l, T r) -> Value {
                                return r == 0 ? null : Value(l / r);
                            },
                            [](auto&&...) -> Value { panic(); },
                        },
                        vl->value.variant(),
                        vr->value.variant());
                case BinaryExprType::Add:
                    return std::visit(
                        util::Overloaded{
                            []<Addable T>(T l, T r) -> Value {
                                using U = std::conditional_t<
                                    std::same_as<T, std::string_view>,
                                    std::string,
                                    T>;

                                return U(l) + U(r);
                            },
                            [](auto&&...) -> Value { panic(); },
                        },
                        vl->value.variant(),
                        vr->value.variant());

                case BinaryExprType::Subtract:
                    return std::visit(
                        util::Overloaded{
                            []<Subtractable T>(T l, T r) -> Value { return l - r; },
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
        auto step_name = "const_scalar_fold";
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

ir::Program constScalarFold(ir::Program program, Context& ctx) {
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

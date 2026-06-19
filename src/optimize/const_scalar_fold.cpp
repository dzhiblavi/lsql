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

        return func::buildScalar<ir::Scalar>(s.function, [&]<typename E>(E executor) {
            ctx.setChanges().note("folded constant scalar function call");
            auto result = execute(executor, values);

            return ir::Scalar{
                .node = ir::ValueScalar{.value = std::move(result)},
                .value_type = self.value_type,
            };
        });
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

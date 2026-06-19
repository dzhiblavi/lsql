#include "optimize/const_aggregate_fold.h"
#include "ir/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    ir::Aggregate optimize(ir::FnCallAggregate& a, auto& self) {
        if (func::isScalar(a.function)) {
            return std::move(self);
        }

        bool all_const = std::ranges::all_of(a.args, [](ir::Scalar& arg) {
            return std::holds_alternative<ir::ValueScalar>(arg.node);
        });

        if (!all_const) {
            return std::move(self);
        }

        if (std::holds_alternative<func::CountNonNull>(a.function)) {
            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            if (value == null) {
                ctx.setChanges().note("UnaryAggregate (count non-null) folded to 0");

                self.node = ir::ConstAggregate{
                    .value = int64_t(0),
                    .null_if_empty = false,
                };
            }
            return std::move(self);
        }

        if (std::holds_alternative<func::Min>(a.function)) {
            ctx.setChanges().note("UnaryAggregate (min) folded");

            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            self.node = ir::ConstAggregate{
                .value = value,
                .null_if_empty = true,
            };
            return std::move(self);
        }

        if (std::holds_alternative<func::Max>(a.function)) {
            ctx.setChanges().note("UnaryAggregate (max) folded");

            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            self.node = ir::ConstAggregate{
                .value = value,
                .null_if_empty = true,
            };
            return std::move(self);
        }

        if (std::holds_alternative<func::Sum>(a.function)) {
            return std::move(self);
        }

        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
        auto self_name = "const_aggregate_fold";
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { optimize(node, self); }) {
            llog::trace("applying {} step for {}", self_name, name);
            return optimize(node, self);
        } else {
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program constAggregateFold(ir::Program program, Context& ctx) {
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

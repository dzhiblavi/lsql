#include "optimize/const_aggregate_fold.h"
#include "ir/Aggregates.h"
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

    void optimize(ir::FnCallAggregate& a, auto& self) {
        if (func::isScalar(a.function)) {
            return;
        }

        bool all_const = std::ranges::all_of(a.args, [](ir::Scalar& arg) {
            return std::holds_alternative<ir::ValueScalar>(arg.node);
        });

        if (!all_const) {
            return;
        }

        if (std::holds_alternative<func::CountNonNull>(a.function)) {
            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            if (value == null) {
                ctx.setChanges().note("UnaryAggregate (count non-null) folded to 0");

                self.node = ir::ConstAggregate{
                    .value = int64_t(0),
                    .null_if_empty = false,
                };
            } else {
                ctx.setChanges().note("count over non-null constant folded to count-all");
                self.node = ir::FnCallAggregate{
                    .function = func::CountAll{},
                    .args = {},
                };
            }
            return;
        }

        if (std::holds_alternative<func::Min>(a.function)) {
            ctx.setChanges().note("UnaryAggregate (min) folded");

            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            self.node = ir::ConstAggregate{
                .value = value,
                .null_if_empty = true,
            };
            return;
        }

        if (std::holds_alternative<func::Max>(a.function)) {
            ctx.setChanges().note("UnaryAggregate (max) folded");

            auto value = std::get<ir::ValueScalar>(a.args[0].node).value;
            self.node = ir::ConstAggregate{
                .value = value,
                .null_if_empty = true,
            };
            return;
        }
    }

    void visit(ir::Aggregate& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { optimize(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying const_aggregate_fold step for {}", name);
                optimize(node, self);
            }
        });
    }
};

}  // namespace

ir::Program constAggregateFold(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

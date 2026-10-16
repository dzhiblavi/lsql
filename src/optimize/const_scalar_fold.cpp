#include "optimize/const_scalar_fold.h"

#include "ir/Aggregates.h"
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "core/function/build.h"
#include "util/TreeWalker.h"
#include "util/logging.h"

#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void optimize(ir::FnCallScalar& s, auto& self) {
        if (!func::isScalar(s.function)) {
            return;
        }

        bool all_const = std::ranges::all_of(s.args, [](ir::Scalar& arg) {
            return std::holds_alternative<ir::ValueScalar>(arg.node);
        });

        if (!all_const) {
            return;
        }

        std::vector<Value> values;
        values.reserve(s.args.size());
        for (auto&& arg : s.args) {
            values.push_back(std::get<ir::ValueScalar>(arg.node).value);
        }

        self = func::buildScalar<ir::Scalar>(s.function, [&]<typename E>(E executor) {
            ctx.setChanges().note("folded constant scalar function call");
            auto result = execute(executor, values);

            return ir::Scalar{
                .node = ir::ValueScalar{.value = std::move(result)},
                .value_type = self.value_type,
            };
        });
    }

    void visit(ir::Scalar& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { optimize(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying const_scalar_fold step for {}", name);
                optimize(node, self);
            }
        });
    }
};

}  // namespace

ir::Program constScalarFold(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

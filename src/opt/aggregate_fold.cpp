#include "opt/aggregate_fold.h"
#include "opt/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ConsumePass<Optimizer> {
    Context& ctx;

    ir::Aggregate optimize(ir::UnaryAggregate& a, auto& self) {
        auto* v = std::get_if<ir::ValueScalar>(&a.expr->node);
        if (v == nullptr) {
            return std::move(self);
        }

        switch (a.type) {
            case UnaryAggregateType::Count:
                if (v->value == false || v->value == null) {
                    ctx.setChanges().note("UnaryAggregate (count) folded to 0");
                    self.node = ir::ConstAggregate{
                        .value = int64_t(0),
                        .null_if_empty = false,
                    };
                }
                return std::move(self);

            case UnaryAggregateType::Min:
                ctx.setChanges().note("UnaryAggregate (min) folded");
                self.node = ir::ConstAggregate{
                    .value = v->value,
                    .null_if_empty = true,
                };
                return std::move(self);

            case UnaryAggregateType::Max:
                ctx.setChanges().note("UnaryAggregate (max) folded");
                self.node = ir::ConstAggregate{
                    .value = v->value,
                    .null_if_empty = true,
                };
                return std::move(self);

            case UnaryAggregateType::Sum:
                return std::move(self);
        }
    }

    auto construct(auto& node, auto& self) {
        auto self_name = "aggregate_fold";
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

ir::Program aggregateFold(ir::Program program, Context& ctx) {
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

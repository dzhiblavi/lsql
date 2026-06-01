#include "opt/aggregate_fold.h"
#include "opt/pass.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer : ConsumePass<Optimizer> {
    ir::Aggregate optimize(ir::UnaryAggregate& a, auto& self) {
        auto* v = std::get_if<ir::ValueScalar>(&a.expr->node);
        if (v == nullptr) {
            return std::move(self);
        }

        switch (a.type) {
            case UnaryAggregateType::Count:
                if (v->value == false || v->value == null) {
                    self.node = ir::ConstAggregate{
                        .value = int64_t(0),
                        .null_if_empty = false,
                    };
                }
                return std::move(self);

            case UnaryAggregateType::Min:
                self.node = ir::ConstAggregate{
                    .value = v->value,
                    .null_if_empty = true,
                };
                return std::move(self);

            case UnaryAggregateType::Max:
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

ir::Program aggregateFold(ir::Program program) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    Optimizer opt;
    for (auto&& statement : program.statements) {
        result.statements.push_back(opt.pass(std::move(statement)));
        // result.statements.push_back(pass(std::move(statement), opt));
    }

    return result;
}

}  // namespace lsql::opt

#include "opt/optimize.h"
#include "opt/pass.h"

#include "core/valueCast.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct Optimizer {
    bool isForwardingProjection(const ir::ProjectionRelation& p) {
        for (auto&& proj : p.projectors) {
            auto* field_expr = std::get_if<ir::FieldScalar>(&proj.expr->node);
            if (field_expr == nullptr) {
                // not a field expression: not our case
                return false;
            }

            if (field_expr->field_id != proj.alias_field_id) {
                // field expression does some renaming apparently
                return false;
            }
        }

        // All projectors are identity projectors
        return true;
    }

    ir::Relation optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (isForwardingProjection(rel)) {
            return std::move(*rel.source);
        }

        return std::move(self);
    }

    ir::Relation optimize(ir::FilterRelation& rel, ir::Relation& self) {
        auto* v = std::get_if<ir::ValueScalar>(&rel.condition->node);
        if (!v) {
            return std::move(self);
        }

        if (v->value == true) {
            return std::move(*rel.source);
        }

        // TODO: optimize to empty relation
        return std::move(self);
    }

    ir::Scalar optimize(ir::CoalesceScalar& s, auto& self) {
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
                    self.node = ir::ValueScalar{.value = std::move(v->value)};
                    return std::move(self);
                }
            }

            // not a constant
            s.args.push_back(std::move(a));
        }

        if (s.args.empty()) {
            self.node = ir::ValueScalar{.value = null};
        }

        return std::move(self);
    }

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

    auto operator()(auto& node, auto& self) {
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { optimize(node, self); }) {
            llog::trace("applying optimization step for {}", name);
            return optimize(node, self);
        } else {
            llog::trace("no rewrite rule for {}", name);
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program optimize(ir::Program program) {
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

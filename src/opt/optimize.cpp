#include "opt/optimize.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "core/valueCast.h"

namespace lsql::opt {

namespace {

ir::Relation optimizeRelation(ir::Relation rel);
ir::Statement optimizeStatement(ir::Statement st);
ir::Scalar optimizeScalar(ir::Scalar sc);
ir::Aggregate optimizeAggregate(ir::Aggregate ag);

std::vector<ir::Projector> optimizeProjectors(std::vector<ir::Projector> ps) {
    std::vector<ir::Projector> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back({
            .alias_field_id = p.alias_field_id,
            .expr = box(optimizeScalar(std::move(*p.expr))),
        });
    }
    return r;
}

std::vector<ir::Scalar> optimizeScalars(std::vector<ir::Scalar> ps) {
    std::vector<ir::Scalar> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back(optimizeScalar(std::move(p)));
    }
    return r;
}

std::vector<ir::Aggregate> optimizeAggregates(std::vector<ir::Aggregate> ps) {
    std::vector<ir::Aggregate> r;
    r.reserve(ps.size());
    for (auto&& p : ps) {
        r.push_back(optimizeAggregate(std::move(p)));
    }
    return r;
}

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
    auto source = optimizeRelation(std::move(*rel.source));

    if (isForwardingProjection(rel)) {
        return source;
    }

    rel.source = box(std::move(source));
    rel.projectors = optimizeProjectors(std::move(rel.projectors));
    return std::move(self);
}

ir::Relation optimize(ir::AggregateRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.aggregates = optimizeAggregates(std::move(rel.aggregates));
    return std::move(self);
}

ir::Relation optimize(ir::GroupRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.aggregates = optimizeAggregates(std::move(rel.aggregates));
    rel.group_list = optimizeProjectors(std::move(rel.group_list));
    return std::move(self);
}

ir::Relation optimize(ir::LimitRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    return std::move(self);
}

ir::Relation optimize(ir::FilterRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.condition = box(optimizeScalar(std::move(*rel.condition)));
    // TODO: if always true/always false then just source
    return std::move(self);
}

ir::Relation optimize(ir::SortRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.order_list = optimizeScalars(std::move(rel.order_list));
    return std::move(self);
}

ir::Relation optimize(ir::TopKRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.order_list = optimizeScalars(std::move(rel.order_list));
    return std::move(self);
}

ir::Relation optimize(ir::SemiJoinRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.match = box(optimizeRelation(std::move(*rel.match)));
    rel.expr = box(optimizeScalar(std::move(*rel.expr)));
    return std::move(self);
}

ir::Relation optimize(ir::MarkJoinRelation& rel, ir::Relation& self) {
    rel.source = box(optimizeRelation(std::move(*rel.source)));
    rel.match = box(optimizeRelation(std::move(*rel.match)));
    rel.expr = box(optimizeScalar(std::move(*rel.expr)));
    return std::move(self);
}

ir::Relation optimize(ir::UnionAllRelation& rel, ir::Relation& self) {
    rel.left = box(optimizeRelation(std::move(*rel.left)));
    rel.right = box(optimizeRelation(std::move(*rel.right)));
    return std::move(self);
}

ir::Relation optimize(ir::UnionAllSortedByRelation& rel, ir::Relation& self) {
    rel.left = box(optimizeRelation(std::move(*rel.left)));
    rel.right = box(optimizeRelation(std::move(*rel.right)));
    rel.order_list = optimizeScalars(std::move(rel.order_list));
    return std::move(self);
}

ir::Relation optimize(ir::MaterializeRelation& rel, ir::Relation& self) {
    rel.relation = box(optimizeRelation(std::move(*rel.relation)));
    return std::move(self);
}

ir::Statement optimize(ir::NamedRelationStatement& st, auto& /*self*/) {
    return ir::NamedRelationStatement{
        .name = st.name,
        .relation = box(optimizeRelation(std::move(*st.relation))),
    };
}

ir::Scalar optimize(ir::CoalesceScalar& s, auto& self) {
    auto args = std::move(s.args);
    s.args.reserve(args.size());

    for (auto&& a : args) {
        auto arg = optimizeScalar(std::move(a));

        if (auto* v = std::get_if<ir::ValueScalar>(&arg.node)) {
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
        s.args.push_back(std::move(arg));
    }

    if (s.args.empty()) {
        self.node = ir::ValueScalar{.value = null};
    }

    return std::move(self);
}

ir::Scalar optimize(ir::CastScalar& s, auto& self) {
    s.expr = box(optimizeScalar(std::move(*s.expr)));
    if (auto* v = std::get_if<ir::ValueScalar>(&s.expr->node)) {
        self.node = ir::ValueScalar{.value = valueCast(std::move(v->value), s.cast_to)};
    }
    return std::move(self);
}

ir::Scalar optimize(ir::LikeScalar& s, auto& self) {
    s.expr = box(optimizeScalar(std::move(*s.expr)));
    // TODO: perform actual Like on constant values
    return std::move(self);
}

ir::Scalar optimize(ir::RSubstrScalar& s, auto& self) {
    s.expr = box(optimizeScalar(std::move(*s.expr)));
    // TODO: perform actual RSubstr on constant values
    return std::move(self);
}

ir::Scalar optimize(ir::UnaryScalar& s, auto& self) {
    s.expr = box(optimizeScalar(std::move(*s.expr)));

    if (auto* v = std::get_if<ir::ValueScalar>(&s.expr->node)) {
        auto value = [&] -> Value {
            switch (s.type) {
                case UnaryExprType::BooleanNegate:
                    return !v->value.get<bool>();
            }
        }();

        self.node = ir::ValueScalar{.value = value};
    }

    return std::move(self);
}

ir::Scalar optimize(ir::BinaryScalar& s, auto& self) {
    s.left = box(optimizeScalar(std::move(*s.left)));
    s.right = box(optimizeScalar(std::move(*s.right)));

    auto* vl = std::get_if<ir::ValueScalar>(&s.left->node);
    auto* vr = std::get_if<ir::ValueScalar>(&s.right->node);

    if (vl && vr) {
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
    }

    return std::move(self);
}

ir::Statement optimize(ir::QueryStatement& st, auto& /*self*/) {
    return ir::QueryStatement{
        .relation = box(optimizeRelation(std::move(*st.relation))),
    };
}

ir::Relation optimizeRelation(ir::Relation rel) {
    return util::match(rel.node, [&](auto& r) -> ir::Relation {
        if constexpr (requires { optimize(r, rel); }) {
            return optimize(r, rel);
        } else {
            return std::move(rel);
        }
    });
}

ir::Statement optimizeStatement(ir::Statement st) {
    return util::match(st, [&](auto& s) -> ir::Statement {
        if constexpr (requires { optimize(s, st); }) {
            return optimize(s, st);
        } else {
            return std::move(st);
        }
    });
}

ir::Scalar optimizeScalar(ir::Scalar sc) {
    return util::match(sc.node, [&](auto& s) -> ir::Scalar {
        if constexpr (requires { optimize(s, sc); }) {
            return optimize(s, sc);
        } else {
            return std::move(sc);
        }
    });
}

ir::Aggregate optimizeAggregate(ir::Aggregate ag) {
    return util::match(ag.node, [&](auto& s) -> ir::Aggregate {
        if constexpr (requires { optimize(s, ag); }) {
            return optimize(s, ag);
        } else {
            return std::move(ag);
        }
    });
}

}  // namespace

ir::Program optimize(ir::Program program) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    for (auto&& statement : program.statements) {
        result.statements.push_back(optimizeStatement(std::move(statement)));
    }

    return result;
}

}  // namespace lsql::opt

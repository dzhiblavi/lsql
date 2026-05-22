#include "iface/sql/bind/lower.h"

#include "iface/sql/bind/Expr.h"
#include "iface/sql/bind/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bind/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bind/Statement.h"    // IWYU pragma: keep

#include "ir/Expressions.h"  // IWYU pragma: keep
#include "ir/Relations.h"    // IWYU pragma: keep

#include "core/require.h"
#include "util/Pinned.h"

#include <magic_enum/magic_enum.hpp>

#include <algorithm>

namespace lsql::iface::sql::bind {

namespace {

UnaryAggregateExprType unaryAggregateType(FnCallExpr::Type type) {
    switch (type) {
        case FnCallExpr::Type::Count:
            return UnaryAggregateExprType::Count;
        case FnCallExpr::Type::Min:
            return UnaryAggregateExprType::Min;
        case FnCallExpr::Type::Max:
            return UnaryAggregateExprType::Max;
        case FnCallExpr::Type::Sum:
            return UnaryAggregateExprType::Sum;
        default:
            panic();
    }
}

bool isUnaryAggregateFunc(FnCallExpr::Type type) {
    switch (type) {
        case FnCallExpr::Type::Count:
        case FnCallExpr::Type::Min:
        case FnCallExpr::Type::Max:
        case FnCallExpr::Type::Sum:
            return true;

        case FnCallExpr::Type::Percentile:
        case FnCallExpr::Type::Coalesce:
        case FnCallExpr::Type::RSubstr:
            return false;
    }
}

class Lowerer {
 public:
    Lowerer() = default;

    ir::Program lowerToIR(Program program) && {
        binding_ = program.binding;
        program_.field_binding = binding_;
        program_.statements.reserve(program.statements.size());

        for (auto&& statement : program.statements) {
            program_.statements.push_back(bindStatement(std::move(statement)));
        }

        return std::move(program_);
    }

 private:
    ir::Statement bindStatement(Statement r) {
        return util::match(std::move(r), [this](auto r) { return bindStatement(std::move(r)); });
    }

    ir::Relation bindRelation(Relation r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindRelation(std::move(node), r); });
    }

    ir::Expr bindExpr(Expr r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindExpr(std::move(node), r); });
    }

    ir::Statement bindStatement(QueryStatement s) {
        auto fields = s.relation->fields_out->subtreeFieldSet();  // aka SELECT *
        auto r = bindRelation(std::move(*s.relation));

        return ir::QueryStatement{
            .relation = box(std::move(r)),
            .fields_out = fields,
        };
    }

    ir::Statement bindStatement(NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = std::make_unique<ir::Relation>(bindRelation(std::move(*s.relation)));
        named_relations_[s.name] = relation.get();

        return ir::NamedRelationStatement{
            .name = s.name,
            .relation = std::move(relation),
        };
    }

    ir::Relation bindRelation(AdhocRelation r, auto& /*info*/) {
        return ir::ValuesRelation{
            .values = std::move(r.values),
            .output_id = r.output_field_id,
        };
    }

    ir::Relation bindRelation(SelectRelation r, auto& /*info*/) {
        auto visible_fields = r.source->fields_out->subtreeFieldSet();
        auto scope = scopedRelation(bindRelation(std::move(*r.source)));

        auto _ = scopedFieldSet(&visible_fields);
        auto projectors = bindProjectors(std::move(r.projectors));

        if (r.where) {
            util::match(
                std::move(r.where->condition->node),
                [&](InExpr e) {
                    // Kind of an optimization over MarkJoinRelation
                    auto key = [&] {
                        auto set = e.match->fields_out->fieldSet();
                        auto _ = scopedFieldSet(&set);
                        // expression only sees fields from `e.match`
                        return bindExpr(std::move(*e.expr));
                    }();

                    auto match = bindRelation(std::move(*e.match));
                    auto source = pullRelation();

                    setRelation(
                        ir::SemiJoinRelation{
                            .source = std::make_unique<ir::Relation>(std::move(source)),
                            .match = std::make_unique<ir::Relation>(std::move(match)),
                            .expr = std::make_unique<ir::Expr>(std::move(key)),
                            .match_field_id = e.match_field_id,
                        });
                },
                [&](auto e) {
                    auto cond = bindExpr(std::move(e), *r.where->condition);
                    auto source = pullRelation();

                    setRelation(
                        ir::FilterRelation{
                            .source = std::make_unique<ir::Relation>(std::move(source)),
                            .condition = std::make_unique<ir::Expr>(std::move(cond)),
                        });
                });
        }

        if (r.group_by.has_value()) {
            auto group_key = bindProjectors(std::move(r.group_by->group_list));
            auto source = pullRelation();

            setRelation(
                ir::GroupRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                    .group_list = std::move(group_key),
                });

            visible_fields.merge(outputFieldsOf(group_key));
        } else if (r.aggregate) {
            auto source = pullRelation();

            setRelation(
                ir::AggregateRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                });
        } else {
            auto source = pullRelation();

            setRelation(
                ir::ProjectionRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .projectors = std::move(projectors),
                });
        }

        visible_fields.merge(outputFieldsOf(projectors));

        if (r.order_by) {
            auto order_list = bindExprs(std::move(r.order_by->order_list));
            auto source = pullRelation();

            setRelation(
                ir::SortRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .order_list = std::move(order_list),
                    .desc = r.order_by->desc,
                });
        }

        if (r.limit) {
            auto source = pullRelation();

            setRelation(
                ir::LimitRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .limit = r.limit->limit,
                });
        }

        return pullRelation();
    }

    ir::Relation bindRelation(UnionAllRelation r, auto& /*info*/) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));

        return ir::UnionAllRelation{
            .left = std::make_unique<ir::Relation>(std::move(left)),
            .right = std::make_unique<ir::Relation>(std::move(right)),
        };
    }

    ir::Relation bindRelation(UnionAllSortedByRelation r, auto& /*info*/) {
        auto field_set =
            FieldSet::merge(r.left->fields_out->fieldSet(), r.right->fields_out->fieldSet());
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));

        auto _ = scopedFieldSet(&field_set);
        auto order_list = bindExprs(std::move(r.order_by.order_list));

        return ir::UnionAllSortedByRelation{
            .left = std::make_unique<ir::Relation>(std::move(left)),
            .right = std::make_unique<ir::Relation>(std::move(right)),
            .order_list = std::move(order_list),
            .desc = r.order_by.desc,
        };
    }

    ir::Relation bindRelation(FileRelation r, auto& info) {
        auto fields = info.fields_out->fieldSet();
        llog::info("path={}, requested fields: {}", r.path, to_string(fields, *binding_));

        return ir::FileRelation{
            .path = std::move(r.path),
            .requested_fields = fields,
        };
    }

    ir::Relation bindRelation(FileIntervalRelation r, auto& info) {
        auto fields = info.fields_out->fieldSet();
        llog::info(
            "(interval) path={}, requested fields: {}", r.path, to_string(fields, *binding_));

        return ir::FileIntervalRelation{
            .path = std::move(r.path),
            .ts_from = r.ts_from,
            .ts_to = r.ts_to,
            .requested_fields = fields,
        };
    }

    ir::Relation bindRelation(NamedRelationReferenceRelation r, auto& /*info*/) {
        return ir::NamedRelationReferenceRelation{
            .name = std::move(r.name),
        };
    }

    ir::Relation bindRelation(MaterializeRelation r, auto& /*info*/) {
        auto arg = bindRelation(std::move(*r.relation));
        return ir::MaterializeRelation{
            .relation = std::make_unique<ir::Relation>(std::move(arg)),
        };
    }

    ir::Expr bindExpr(IdentifierExpr e, auto& info) {
        return {
            .node =
                ir::FieldExpr{
                    .field_id = e.field_id,
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(LiteralExpr e, auto& info) {
        return {
            .node =
                ir::ValueExpr{
                    .value = e.value,
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(CastExpr e, auto& info) {
        return {
            .node =
                ir::CastExpr{
                    .cast_to = e.cast_to,
                    .expr = box<ir::Expr>(bindExpr(std::move(*e.expr))),
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(InExpr e, auto& /*info*/) {
        auto output_id = binding_->addAnonymous(ValueType::Boolean);
        auto expr = bindExpr(std::move(*e.expr));
        auto match = bindRelation(std::move(*e.match));

        // same source relation enriched with a boolean field indicating
        // whether the row's key matches `match`
        setRelation(
            ir::MarkJoinRelation{
                .source = std::make_unique<ir::Relation>(pullRelation()),
                .match = std::make_unique<ir::Relation>(std::move(match)),
                .expr = std::make_unique<ir::Expr>(std::move(expr)),
                .output_field_id = output_id,
                .match_field_id = e.match_field_id,
            });

        return {
            .node =
                ir::FieldExpr{
                    .field_id = output_id,
                },
            .value_type = ValueType::Boolean,
            .level = ExprKindLevel::Row,
        };
    }

    ir::Expr bindExpr(LikeExpr e, auto& info) {
        auto arg = bindExpr(std::move(*e.expr));

        return {
            .node =
                ir::LikeExpr{
                    .expr = std::make_unique<ir::Expr>(std::move(arg)),
                    .regex = std::move(e.regex),
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(FnCallExpr e, auto& info) {
        std::vector<ir::Expr> args;
        args.reserve(e.args.size());
        for (auto&& arg : e.args) {
            args.push_back(bindExpr(std::move(arg)));
        }

        if (isUnaryAggregateFunc(e.type)) {
            return {
                .node =
                    ir::UnaryAggregateExpr{
                        .type = unaryAggregateType(e.type),
                        .expr = std::make_unique<ir::Expr>(std::move(args[0])),
                    },
                .value_type = info.value_type,
                .level = info.level,
            };
        }

        if (e.type == FnCallExpr::Type::Coalesce) {
            return {
                .node =
                    ir::CoalesceExpr{
                        .args = std::move(args),
                    },
                .value_type = info.value_type,
                .level = info.level,
            };
        }

        if (e.type == FnCallExpr::Type::Percentile) {
            std::vector<float> percentiles;
            percentiles.reserve(args.size() - 1);
            for (size_t i = 1; i < args.size(); ++i) {
                percentiles.push_back(
                    util::match(
                        std::move(args[i].node),
                        [](ir::ValueExpr e) -> float {
                            float p = e.value.get<float>();
                            require(0.f <= p && p <= 1.f, "percentiles must be in [0.0, 1.0]");
                            return p;
                        },
                        [](auto) -> float { panic(); }));
            }

            return {
                .node =
                    ir::PercentileExpr{
                        .expr = std::make_unique<ir::Expr>(std::move(args[0])),
                        .percentiles = std::move(percentiles),
                    },
                .value_type = info.value_type,
                .level = info.level,
            };
        }

        if (e.type == FnCallExpr::Type::RSubstr) {
            std::string regex = util::match(
                args[1].node,
                [](ir::ValueExpr e) { return e.value.get<std::string>(); },
                [](auto&&) -> std::string { panic(); });

            return {
                .node =
                    ir::RSubstrExpr{
                        .expr = std::make_unique<ir::Expr>(std::move(args[0])),
                        .regex = std::move(regex),
                    },
                .value_type = info.value_type,
                .level = info.level,
            };
        }

        panic();
    }

    ir::Expr bindExpr(BinaryExpr e, auto& info) {
        auto left = bindExpr(std::move(*e.left));
        auto right = bindExpr(std::move(*e.right));

        return {
            .node =
                ir::BinaryExpr{
                    .type = e.type,
                    .left = std::make_unique<ir::Expr>(std::move(left)),
                    .right = std::make_unique<ir::Expr>(std::move(right)),
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(UnaryExpr e, auto& info) {
        auto arg = bindExpr(std::move(*e.expr));

        return {
            .node =
                ir::UnaryExpr{
                    .type = e.type,
                    .expr = std::make_unique<ir::Expr>(std::move(arg)),
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    void bind(Projector p, std::vector<ir::Projector>& out) {
        util::match(
            std::move(p),
            [&](StarProjector) {
                for (auto id : currFieldSet().fieldIds()) {
                    out.push_back(
                        ir::Projector{
                            .alias_field_id = id,
                            .expr = box<ir::Expr>(ir::Expr{
                                .node =
                                    ir::FieldExpr{
                                        .field_id = id,
                                    },
                                .value_type = binding_->type(id),
                                .level = ExprKindLevel::Row,
                            }),
                        });
                }
            },
            [&](IdentifierProjector p) {
                out.push_back(
                    ir::Projector{
                        .alias_field_id = p.field_id,
                        .expr = box<ir::Expr>(ir::Expr{
                            .node =
                                ir::FieldExpr{
                                    .field_id = p.field_id,
                                },
                            .value_type = binding_->type(p.field_id),
                            .level = ExprKindLevel::Row,
                        }),
                    });
            },
            [&](ExprProjector p) {
                out.push_back(
                    ir::Projector{
                        .alias_field_id = p.alias_field_id,
                        .expr = box<ir::Expr>(bindExpr(std::move(*p.expr))),
                    });
            });
    }

    std::vector<ir::Projector> bindProjectors(std::vector<Projector> projectors) {
        std::vector<ir::Projector> result;
        result.reserve(projectors.size());
        for (auto&& p : projectors) {
            bind(std::move(p), result);
        }
        return result;
    }

    std::vector<ir::Expr> bindExprs(std::vector<Expr> exprs) {
        std::vector<ir::Expr> result;
        result.reserve(exprs.size());
        for (auto&& p : exprs) {
            result.push_back(bindExpr(std::move(p)));
        }
        return result;
    }

    FieldSet outputFieldsOf(const std::vector<ir::Projector>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.add(p.alias_field_id);
        }
        return fields;
    }

    struct ScopedRelation : util::Pinned {
        std::optional<ir::Relation>* slot;
        std::optional<ir::Relation> old;
        ~ScopedRelation() { *slot = std::move(old); }
    };

    ScopedRelation scopedRelation(ir::Relation curr) {
        return ScopedRelation{
            .slot = &curr_relation_slot_,
            .old = std::exchange(curr_relation_slot_, std::move(curr)),
        };
    }

    ir::Relation& currRelation() {
        verify(curr_relation_slot_.has_value());
        return *curr_relation_slot_;
    }

    ir::Relation pullRelation() {
        auto r = std::move(currRelation());
        curr_relation_slot_ = std::nullopt;
        return r;
    }

    void setRelation(ir::Relation r) {
        verify(!curr_relation_slot_.has_value());
        curr_relation_slot_.emplace(std::move(r));
    }

    struct ScopedFieldSet : util::Pinned {
        const FieldSet** slot;
        const FieldSet* old;
        ~ScopedFieldSet() { *slot = old; }
    };

    ScopedFieldSet scopedFieldSet(const FieldSet* curr) {
        return ScopedFieldSet{
            .slot = &curr_field_set_slot_,
            .old = std::exchange(curr_field_set_slot_, curr),
        };
    }

    const FieldSet& currFieldSet() {
        verify(curr_field_set_slot_);
        return *curr_field_set_slot_;
    }

    ir::Program program_;
    FieldBindingPtr binding_;
    std::unordered_map<std::string, ir::Relation*> named_relations_;
    std::optional<ir::Relation> curr_relation_slot_;
    const FieldSet* curr_field_set_slot_ = nullptr;
};

}  // namespace

ir::Program lowerToIR(Program program) {
    return Lowerer().lowerToIR(std::move(program));
}

}  // namespace lsql::iface::sql::bind

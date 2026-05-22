#include "iface/sql/lower/lower.h"

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

namespace lsql::iface::sql::lower {

namespace {

UnaryAggregateExprType unaryAggregateType(bind::FnCallExpr::Type type) {
    switch (type) {
        case bind::FnCallExpr::Type::Count:
            return UnaryAggregateExprType::Count;
        case bind::FnCallExpr::Type::Min:
            return UnaryAggregateExprType::Min;
        case bind::FnCallExpr::Type::Max:
            return UnaryAggregateExprType::Max;
        case bind::FnCallExpr::Type::Sum:
            return UnaryAggregateExprType::Sum;
        default:
            panic();
    }
}

bool isUnaryAggregateFunc(bind::FnCallExpr::Type type) {
    switch (type) {
        case bind::FnCallExpr::Type::Count:
        case bind::FnCallExpr::Type::Min:
        case bind::FnCallExpr::Type::Max:
        case bind::FnCallExpr::Type::Sum:
            return true;

        case bind::FnCallExpr::Type::Percentile:
        case bind::FnCallExpr::Type::Coalesce:
        case bind::FnCallExpr::Type::RSubstr:
            return false;
    }
}

class Lowerer {
 public:
    Lowerer() = default;

    ir::Program lowerToIR(bind::Program program) && {
        binding_ = program.binding;
        program_.field_binding = binding_;
        program_.statements.reserve(program.statements.size());

        for (auto&& statement : program.statements) {
            program_.statements.push_back(bindStatement(std::move(statement)));
        }

        return std::move(program_);
    }

 private:
    ir::Statement bindStatement(bind::Statement r) {
        return util::match(std::move(r), [this](auto r) { return bindStatement(std::move(r)); });
    }

    ir::Relation bindRelation(bind::Relation r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindRelation(std::move(node), r); });
    }

    ir::Expr bindExpr(bind::Expr r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindExpr(std::move(node), r); });
    }

    ir::Statement bindStatement(bind::QueryStatement s) {
        auto r = bindRelation(std::move(*s.relation));
        return ir::QueryStatement{.relation = box(std::move(r))};
    }

    ir::Statement bindStatement(bind::NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = std::make_unique<ir::Relation>(bindRelation(std::move(*s.relation)));
        named_relations_[s.name] = relation.get();

        return ir::NamedRelationStatement{
            .name = s.name,
            .relation = std::move(relation),
        };
    }

    ir::Relation bindRelation(bind::AdhocRelation r, auto& info) {
        return {
            .node =
                ir::ValuesRelation{
                    .values = std::move(r.values),
                    .output_id = r.output_field_id,
                },
            .fields_out = info.fields_out->fieldSet(),
        };
    }

    ir::Relation bindRelation(bind::SelectRelation r, auto& /*info*/) {
        auto scope = scopedRelation(bindRelation(std::move(*r.source)));

        auto visible_fields = currRelation().fields_out;
        auto _ = scopedFieldSet(&visible_fields);
        auto projectors = bindProjectors(std::move(r.projectors));

        if (r.where) {
            util::match(
                std::move(r.where->condition->node),
                [&](bind::InExpr e) {
                    auto match = bindRelation(std::move(*e.match));

                    // Kind of an optimization over MarkJoinRelation
                    auto key = [&] {
                        // expression only sees fields from `match`
                        auto _ = scopedFieldSet(&match.fields_out);
                        return bindExpr(std::move(*e.expr));
                    }();

                    auto source = pullRelation();

                    // output fields are same as source
                    auto fields_out = source.fields_out;

                    setRelation({
                        .node =
                            ir::SemiJoinRelation{
                                .source = std::make_unique<ir::Relation>(std::move(source)),
                                .match = std::make_unique<ir::Relation>(std::move(match)),
                                .expr = std::make_unique<ir::Expr>(std::move(key)),
                                .match_field_id = e.match_field_id,
                            },
                        .fields_out = fields_out,
                    });
                },
                [&](auto e) {
                    auto cond = bindExpr(std::move(e), *r.where->condition);
                    auto source = pullRelation();
                    // output fields are same as source
                    auto fields_out = source.fields_out;

                    setRelation({
                        .node =
                            ir::FilterRelation{
                                .source = std::make_unique<ir::Relation>(std::move(source)),
                                .condition = std::make_unique<ir::Expr>(std::move(cond)),
                            },
                        .fields_out = fields_out,
                    });
                });
        }

        if (r.group_by.has_value()) {
            auto group_key = bindProjectors(std::move(r.group_by->group_list));
            auto source = pullRelation();
            auto group_fields = outputFieldsOf(group_key);
            auto fields = outputFieldsOf(projectors);

            setRelation({
                .node =
                    ir::GroupRelation{
                        .source = std::make_unique<ir::Relation>(std::move(source)),
                        .projectors = std::move(projectors),
                        .group_list = std::move(group_key),
                    },
                .fields_out = fields,
            });

            visible_fields.merge(group_fields);
        } else if (r.aggregate) {
            auto source = pullRelation();
            auto fields = outputFieldsOf(projectors);

            setRelation({
                .node =
                    ir::AggregateRelation{
                        .source = std::make_unique<ir::Relation>(std::move(source)),
                        .projectors = std::move(projectors),
                    },
                .fields_out = fields,
            });
        } else {
            auto source = pullRelation();
            auto fields = outputFieldsOf(projectors);

            setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = std::make_unique<ir::Relation>(std::move(source)),
                        .projectors = std::move(projectors),
                    },
                .fields_out = fields,
            });
        }

        visible_fields.merge(outputFieldsOf(projectors));

        if (r.order_by) {
            auto order_list = bindExprs(std::move(r.order_by->order_list));
            auto source = pullRelation();
            auto fields = source.fields_out;

            setRelation({
                .node =
                    ir::SortRelation{
                        .source = std::make_unique<ir::Relation>(std::move(source)),
                        .order_list = std::move(order_list),
                        .desc = r.order_by->desc,
                    },
                .fields_out = fields,
            });
        }

        if (r.limit) {
            auto source = pullRelation();
            auto fields = source.fields_out;

            setRelation({
                .node =
                    ir::LimitRelation{
                        .source = std::make_unique<ir::Relation>(std::move(source)),
                        .limit = r.limit->limit,
                    },
                .fields_out = fields,
            });
        }

        return pullRelation();
    }

    ir::Relation bindRelation(bind::UnionAllRelation r, auto& /*info*/) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = FieldSet::merge(left.fields_out, right.fields_out);

        return {
            .node =
                ir::UnionAllRelation{
                    .left = std::make_unique<ir::Relation>(std::move(left)),
                    .right = std::make_unique<ir::Relation>(std::move(right)),
                },
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::UnionAllSortedByRelation r, auto& /*info*/) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = FieldSet::merge(left.fields_out, right.fields_out);

        auto _ = scopedFieldSet(&fields);
        auto order_list = bindExprs(std::move(r.order_by.order_list));

        return {
            .node =
                ir::UnionAllSortedByRelation{
                    .left = std::make_unique<ir::Relation>(std::move(left)),
                    .right = std::make_unique<ir::Relation>(std::move(right)),
                    .order_list = std::move(order_list),
                    .desc = r.order_by.desc,
                },
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::FileRelation r, auto& info) {
        auto fields = info.fields_out->fieldSet();
        llog::info("path={}, requested fields: {}", r.path, to_string(fields, *binding_));

        return {
            .node = ir::FileRelation{.path = std::move(r.path)},
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::FileIntervalRelation r, auto& info) {
        auto fields = info.fields_out->fieldSet();
        llog::info(
            "(interval) path={}, requested fields: {}", r.path, to_string(fields, *binding_));

        return {
            .node =
                ir::FileIntervalRelation{
                    .path = std::move(r.path),
                    .ts_from = r.ts_from,
                    .ts_to = r.ts_to,
                },
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::NamedRelationReferenceRelation r, auto& /*info*/) {
        auto it = named_relations_.find(r.name);
        verify(it != named_relations_.end());
        auto fields = it->second->fields_out;

        return {
            .node =
                ir::NamedRelationReferenceRelation{
                    .name = std::move(r.name),
                },
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::MaterializeRelation r, auto& /*info*/) {
        auto arg = bindRelation(std::move(*r.relation));
        auto fields = arg.fields_out;

        return {
            .node =
                ir::MaterializeRelation{
                    .relation = std::make_unique<ir::Relation>(std::move(arg)),
                },
            .fields_out = fields,
        };
    }

    ir::Expr bindExpr(bind::IdentifierExpr e, auto& info) {
        return {
            .node =
                ir::FieldExpr{
                    .field_id = e.field_id,
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(bind::LiteralExpr e, auto& info) {
        return {
            .node =
                ir::ValueExpr{
                    .value = e.value,
                },
            .value_type = info.value_type,
            .level = info.level,
        };
    }

    ir::Expr bindExpr(bind::CastExpr e, auto& info) {
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

    ir::Expr bindExpr(bind::InExpr e, auto& /*info*/) {
        auto output_id = binding_->addAnonymous(ValueType::Boolean);
        auto expr = bindExpr(std::move(*e.expr));
        auto match = bindRelation(std::move(*e.match));

        auto source = pullRelation();
        auto fields = source.fields_out;
        fields.add(output_id);

        // same source relation enriched with a boolean field indicating
        // whether the row's key matches `match`
        setRelation({
            .node =
                ir::MarkJoinRelation{
                    .source = std::make_unique<ir::Relation>(std::move(source)),
                    .match = std::make_unique<ir::Relation>(std::move(match)),
                    .expr = std::make_unique<ir::Expr>(std::move(expr)),
                    .output_field_id = output_id,
                    .match_field_id = e.match_field_id,
                },
            .fields_out = fields,
        });

        return {
            .node = ir::FieldExpr{.field_id = output_id},
            .value_type = ValueType::Boolean,
            .level = ExprKindLevel::Row,
        };
    }

    ir::Expr bindExpr(bind::LikeExpr e, auto& info) {
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

    ir::Expr bindExpr(bind::FnCallExpr e, auto& info) {
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

        if (e.type == bind::FnCallExpr::Type::Coalesce) {
            return {
                .node =
                    ir::CoalesceExpr{
                        .args = std::move(args),
                    },
                .value_type = info.value_type,
                .level = info.level,
            };
        }

        if (e.type == bind::FnCallExpr::Type::Percentile) {
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

        if (e.type == bind::FnCallExpr::Type::RSubstr) {
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

    ir::Expr bindExpr(bind::BinaryExpr e, auto& info) {
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

    ir::Expr bindExpr(bind::UnaryExpr e, auto& info) {
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

    void bind(bind::Projector p, std::vector<ir::Projector>& out) {
        util::match(
            std::move(p),
            [&](bind::StarProjector) {
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
            [&](bind::IdentifierProjector p) {
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
            [&](bind::ExprProjector p) {
                out.push_back(
                    ir::Projector{
                        .alias_field_id = p.alias_field_id,
                        .expr = box<ir::Expr>(bindExpr(std::move(*p.expr))),
                    });
            });
    }

    std::vector<ir::Projector> bindProjectors(std::vector<bind::Projector> projectors) {
        std::vector<ir::Projector> result;
        result.reserve(projectors.size());
        for (auto&& p : projectors) {
            bind(std::move(p), result);
        }
        return result;
    }

    std::vector<ir::Expr> bindExprs(std::vector<bind::Expr> exprs) {
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

ir::Program lowerToIR(bind::Program program) {
    return Lowerer().lowerToIR(std::move(program));
}

}  // namespace lsql::iface::sql::lower

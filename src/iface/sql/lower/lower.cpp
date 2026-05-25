#include "iface/sql/lower/lower.h"

#include "iface/sql/bind/Expr.h"
#include "iface/sql/bind/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bind/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bind/Statement.h"    // IWYU pragma: keep

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "core/require.h"
#include "util/Pinned.h"

#include <magic_enum/magic_enum.hpp>

#include <algorithm>

namespace lsql::iface::sql::lower {

namespace {

template <typename T>
void append(std::vector<T>& a, std::vector<T> b) {
    a.insert(a.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
}

template <typename T>
std::vector<T> concat(std::vector<T> a, std::vector<T> b) {
    append(a, std::move(b));
    return a;
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
    // Expression tree + all its aggregates (leaves with their own expression subtrees)
    using BindExprResult = std::pair<ir::Scalar, std::vector<ir::Aggregate>>;

    ir::Statement bindStatement(bind::Statement r) {
        return util::match(std::move(r), [this](auto r) { return bindStatement(std::move(r)); });
    }

    ir::Relation bindRelation(bind::Relation r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindRelation(std::move(node), r); });
    }

    BindExprResult bindExpr(bind::Expr r) {
        return util::match(
            std::move(r.node), [&](auto node) { return bindExpr(std::move(node), r); });
    }

    ir::Statement bindStatement(bind::QueryStatement s) {
        auto r = bindRelation(std::move(*s.relation));
        return ir::QueryStatement{.relation = box(std::move(r))};
    }

    ir::Statement bindStatement(bind::NamedRelationStatement s) {
        require(!named_relations_.contains(s.name), "duplicate named relation '{}'", s.name);

        auto relation = box(bindRelation(std::move(*s.relation)));
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

        auto [projectors, proj_aggregates] = bindProjectors(std::move(r.projectors));
        auto projectors_output_fields = outputFieldsOf(projectors);
        auto proj_aggregates_output_fields = outputFieldsOf(proj_aggregates);

        if (r.where) {
            util::match(
                std::move(r.where->condition->node),
                [&](bind::InExpr e) {
                    auto match = bindRelation(std::move(*e.match));

                    auto [key, key_aggregates] = [&] {
                        auto _ = scopedFieldSet(&match.fields_out);
                        return bindExpr(std::move(*e.expr));
                    }();
                    verify(key_aggregates.empty());

                    setRelation({
                        .node =
                            ir::SemiJoinRelation{
                                .source = box(pullRelation()),
                                .match = box(std::move(match)),
                                .expr = box(std::move(key)),
                                .match_field_id = e.match_field_id,
                            },
                        .fields_out = visible_fields,
                    });
                },
                [&](auto e) {
                    auto [cond, cond_aggregates] = bindExpr(std::move(e), *r.where->condition);
                    verify(cond_aggregates.empty());

                    setRelation({
                        .node =
                            ir::FilterRelation{
                                .source = box(pullRelation()),
                                .condition = box(std::move(cond)),
                            },
                        .fields_out = visible_fields,
                    });
                });
        }

        if (r.group_by.has_value()) {
            auto [group_key, group_key_aggregates] =
                bindProjectors(std::move(r.group_by->group_list));
            verify(group_key_aggregates.empty());

            auto group_key_output_fields = outputFieldsOf(group_key);

            auto group_rel = ir::Relation{
                .node =
                    ir::GroupRelation{
                        .source = box(pullRelation()),
                        .aggregates = std::move(proj_aggregates),
                        .group_list = std::move(group_key),
                    },
                .fields_out =
                    FieldSet::merge(proj_aggregates_output_fields, group_key_output_fields),
            };

            setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(std::move(group_rel)),
                        .projectors = std::move(projectors),
                    },
                .fields_out = projectors_output_fields,
            });

            visible_fields = FieldSet::merge(projectors_output_fields, group_key_output_fields);
        } else if (r.aggregate) {
            verify(!proj_aggregates.empty());
            auto aggregate = ir::Relation{
                .node =
                    ir::AggregateRelation{
                        .source = box(pullRelation()),
                        .aggregates = std::move(proj_aggregates),
                    },
                .fields_out = proj_aggregates_output_fields,
            };

            setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(std::move(aggregate)),
                        .projectors = std::move(projectors),
                    },
                .fields_out = projectors_output_fields,
            });

            visible_fields = projectors_output_fields;
        } else {
            verify(proj_aggregates.empty());

            setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(pullRelation()),
                        .projectors = std::move(projectors),
                    },
                .fields_out = projectors_output_fields,
            });

            visible_fields = projectors_output_fields;
        }

        if (r.order_by) {
            auto [order_list, order_aggregates] = bindExprs(std::move(r.order_by->order_list));
            verify(order_aggregates.empty());

            setRelation({
                .node =
                    ir::SortRelation{
                        .source = box(pullRelation()),
                        .order_list = std::move(order_list),
                        .desc = r.order_by->desc,
                    },
                .fields_out = projectors_output_fields,
            });
        }

        if (r.limit) {
            setRelation({
                .node =
                    ir::LimitRelation{
                        .source = box(pullRelation()),
                        .limit = r.limit->limit,
                    },
                .fields_out = projectors_output_fields,
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
                    .left = box(std::move(left)),
                    .right = box(std::move(right)),
                },
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::UnionAllSortedByRelation r, auto& /*info*/) {
        auto left = bindRelation(std::move(*r.left));
        auto right = bindRelation(std::move(*r.right));
        auto fields = FieldSet::merge(left.fields_out, right.fields_out);

        auto _ = scopedFieldSet(&fields);
        auto [order_list, aggregates] = bindExprs(std::move(r.order_by.order_list));
        verify(aggregates.empty());

        return {
            .node =
                ir::UnionAllSortedByRelation{
                    .left = box(std::move(left)),
                    .right = box(std::move(right)),
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
            .node = ir::NamedRelationReferenceRelation{.name = std::move(r.name)},
            .fields_out = fields,
        };
    }

    ir::Relation bindRelation(bind::MaterializeRelation r, auto& /*info*/) {
        auto arg = bindRelation(std::move(*r.relation));
        auto fields = arg.fields_out;

        return {
            .node = ir::MaterializeRelation{.relation = box(std::move(arg))},
            .fields_out = fields,
        };
    }

    BindExprResult bindExpr(bind::IdentifierExpr e, auto& info) {
        return BindExprResult(
            {
                .node = ir::FieldScalar{.field_id = e.field_id},
                .value_type = info.value_type,
            },
            {});
    }

    BindExprResult bindExpr(bind::ValueExpr e, auto& info) {
        return BindExprResult(
            {
                .node = ir::ValueScalar{.value = std::move(e.value)},
                .value_type = info.value_type,
            },
            {});
    }

    BindExprResult bindExpr(bind::CastExpr e, auto& info) {
        auto [arg, aggregates] = bindExpr(std::move(*e.expr));

        return BindExprResult(
            {
                .node =
                    ir::CastScalar{
                        .cast_to = e.cast_to,
                        .expr = box(std::move(arg)),
                    },
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    BindExprResult bindExpr(bind::InExpr e, auto& /*info*/) {
        auto output_id = binding_->addAnonymous("mark_join", ValueType::Boolean);
        auto [expr, aggregates] = bindExpr(std::move(*e.expr));
        verify(aggregates.empty());
        auto match = bindRelation(std::move(*e.match));

        auto source = pullRelation();
        auto fields = source.fields_out;
        fields.add(output_id);

        // same source relation enriched with a boolean field indicating
        // whether the row's key matches `match`
        setRelation({
            .node =
                ir::MarkJoinRelation{
                    .source = box(std::move(source)),
                    .match = box(std::move(match)),
                    .expr = box(std::move(expr)),
                    .output_field_id = output_id,
                    .match_field_id = e.match_field_id,
                },
            .fields_out = fields,
        });

        return BindExprResult(
            {
                .node = ir::FieldScalar{.field_id = output_id},
                .value_type = ValueType::Boolean,
            },
            {});
    }

    BindExprResult bindExpr(bind::LikeExpr e, auto& info) {
        auto [arg, aggregates] = bindExpr(std::move(*e.expr));

        return BindExprResult(
            {
                .node =
                    ir::LikeScalar{
                        .expr = box(std::move(arg)),
                        .regex = std::move(e.regex),
                    },
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    BindExprResult bindExpr(bind::UnaryAggregateExpr e, auto& info) {
        auto output_field_id = binding_->addAnonymous("un_agr", info.value_type);
        auto [expr, aggregates] = bindExpr(std::move(*e.expr));
        verify(aggregates.empty());

        std::vector<ir::Aggregate> aggrs;
        aggrs.push_back({
            .node =
                ir::UnaryAggregate{
                    .type = e.type,
                    .expr = box(std::move(expr)),
                },
            .output_field_id = output_field_id,
            .value_type = info.value_type,
        });

        return BindExprResult(
            {
                .node = ir::FieldScalar{.field_id = output_field_id},
                .value_type = info.value_type,
            },
            std::move(aggrs));
    }

    BindExprResult bindExpr(bind::RSubstrExpr e, auto& info) {
        auto [expr, aggregates] = bindExpr(std::move(*e.expr));

        return BindExprResult(
            {
                .node =
                    ir::RSubstrScalar{
                        .expr = box(std::move(expr)),
                        .regex = std::move(e.regex),
                    },
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    BindExprResult bindExpr(bind::CoalesceExpr e, auto& info) {
        auto [exprs, aggregates] = bindExprs(std::move(e.args));

        return BindExprResult(
            {
                .node = ir::CoalesceScalar{.args = std::move(exprs)},
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    BindExprResult bindExpr(bind::PercentileExpr e, auto& info) {
        auto output_field_id = binding_->addAnonymous("perc", ValueType::String);
        auto [expr, aggregates] = bindExpr(std::move(*e.expr));
        verify(aggregates.empty());

        std::vector<ir::Aggregate> aggrs;
        aggrs.push_back({
            .node =
                ir::PercentileAggregate{
                    .expr = box(std::move(expr)),
                    .percentiles = std::move(e.percentiles),
                },
            .output_field_id = output_field_id,
            .value_type = info.value_type,
        });

        return BindExprResult(
            ir::Scalar{
                .node = ir::FieldScalar{.field_id = output_field_id},
                .value_type = ValueType::String,
            },
            std::move(aggrs));
    }

    BindExprResult bindExpr(bind::BinaryExpr e, auto& info) {
        auto [left, al] = bindExpr(std::move(*e.left));
        auto [right, ar] = bindExpr(std::move(*e.right));

        return BindExprResult(
            {
                .node =
                    ir::BinaryScalar{
                        .type = e.type,
                        .left = box(std::move(left)),
                        .right = box(std::move(right)),
                    },
                .value_type = info.value_type,
            },
            concat(std::move(al), std::move(ar)));
    }

    BindExprResult bindExpr(bind::UnaryExpr e, auto& info) {
        auto [arg, aggregates] = bindExpr(std::move(*e.expr));

        return BindExprResult(
            {
                .node =
                    ir::UnaryScalar{
                        .type = e.type,
                        .expr = box(std::move(arg)),
                    },
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    void bind(
        bind::Projector p, std::vector<ir::Projector>& exprs, std::vector<ir::Aggregate>& aggrs) {
        util::match(
            std::move(p),
            [&](bind::StarProjector) {
                for (auto id : currFieldSet().fieldIds()) {
                    exprs.push_back(
                        ir::Projector{
                            .alias_field_id = id,
                            .expr =
                                box(ir::Scalar{
                                    .node = ir::FieldScalar{.field_id = id},
                                    .value_type = binding_->type(id),
                                }),
                        });
                }
            },
            [&](bind::IdentifierProjector p) {
                exprs.push_back(
                    ir::Projector{
                        .alias_field_id = p.field_id,
                        .expr =
                            box(ir::Scalar{
                                .node = ir::FieldScalar{.field_id = p.field_id},
                                .value_type = binding_->type(p.field_id),
                            }),
                    });
            },
            [&](bind::ExprProjector p) {
                auto [expr, aggregates] = bindExpr(std::move(*p.expr));

                exprs.push_back(
                    ir::Projector{
                        .alias_field_id = p.alias_field_id,
                        .expr = box(std::move(expr)),
                    });

                append(aggrs, std::move(aggregates));
            });
    }

    std::pair<std::vector<ir::Projector>, std::vector<ir::Aggregate>> bindProjectors(
        std::vector<bind::Projector> projectors) {
        std::vector<ir::Projector> exprs;
        std::vector<ir::Aggregate> aggrs;
        exprs.reserve(projectors.size());
        for (auto&& p : projectors) {
            bind(std::move(p), exprs, aggrs);
        }
        return std::make_pair(std::move(exprs), std::move(aggrs));
    }

    std::pair<std::vector<ir::Scalar>, std::vector<ir::Aggregate>> bindExprs(
        std::vector<bind::Expr> es) {
        std::vector<ir::Scalar> exprs;
        exprs.reserve(es.size());
        std::vector<ir::Aggregate> aggrs;

        for (auto&& p : es) {
            auto [expr, aggregates] = bindExpr(std::move(p));
            exprs.push_back(std::move(expr));
            append(aggrs, std::move(aggregates));
        }

        return std::make_pair(std::move(exprs), std::move(aggrs));
    }

    FieldSet outputFieldsOf(const std::vector<ir::Projector>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.add(p.alias_field_id);
        }
        return fields;
    }

    FieldSet outputFieldsOf(const std::vector<ir::Aggregate>& ps) {
        auto fields = FieldSet::emptySet();
        for (auto&& p : ps) {
            fields.add(p.output_field_id);
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

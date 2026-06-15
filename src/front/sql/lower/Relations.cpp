#include "front/sql/lower/Relations.h"

#include "front/sql/lower/Expressions.h"

#include "ir/pass.h"
#include "ir/tools.h"

#include "util/containers.h"

namespace lsql::front::sql::lower {

namespace {

struct FieldRefCollector : ir::ScalarViewPass<FieldRefCollector, void> {
    void view(const ir::FieldScalar& s, auto&&) { referenced.add(s.field_id); }
    void view(auto&&...) {}

    FieldSet referenced = FieldSet::emptySet();
};

FieldSet referencedFieldIdsBy(const std::vector<ir::Scalar>& ss) {
    FieldRefCollector collector;
    for (auto&& s : ss) {
        collector.pass(s);
    }
    return collector.referenced;
}

void lowerToIR(
    bound::Projector p,
    std::vector<ir::Projector>& exprs,
    std::vector<ir::Aggregate>& aggrs,
    Context& ctx) {
    util::match(
        std::move(p),
        [&](bound::StarProjector) {
            for (auto id : ctx.currFieldSet().fieldIds()) {
                exprs.push_back(
                    ir::Projector{
                        .alias_field_id = id,
                        .expr =
                            box(ir::Scalar{
                                .node = ir::FieldScalar{.field_id = id},
                                .value_type = ctx.binding()->type(id),
                            }),
                    });
            }
        },
        [&](bound::IdentifierProjector p) {
            exprs.push_back(
                ir::Projector{
                    .alias_field_id = p.field_id,
                    .expr =
                        box(ir::Scalar{
                            .node = ir::FieldScalar{.field_id = p.field_id},
                            .value_type = ctx.binding()->type(p.field_id),
                        }),
                });
        },
        [&](bound::ExprProjector p) {
            auto [expr, aggregates] = lowerToIR(std::move(*p.expr), ctx);

            exprs.push_back(
                ir::Projector{
                    .alias_field_id = p.alias_field_id,
                    .expr = box(std::move(expr)),
                });

            util::append(aggrs, std::move(aggregates));
        });
}

std::pair<std::vector<ir::Projector>, std::vector<ir::Aggregate>> lowerToIR(
    std::vector<bound::Projector> projectors, Context& ctx) {
    std::vector<ir::Projector> exprs;
    std::vector<ir::Aggregate> aggrs;
    exprs.reserve(projectors.size());
    for (auto&& p : projectors) {
        lowerToIR(std::move(p), exprs, aggrs, ctx);
    }
    return std::make_pair(std::move(exprs), std::move(aggrs));
}

ir::Relation lowerToIR(bound::AdhocRelation r, auto& /*self*/, Context& /*ctx*/) {
    auto schema = Schema();
    schema.append(r.output_field_id);

    return {
        .node =
            ir::ValuesRelation{
                .values = std::move(r.values),
                .output_id = r.output_field_id,
            },
        .schema = schema,
    };
}

auto identityProjectorsFor(const std::vector<ir::Projector>& ps, Context& ctx) {
    std::vector<ir::Projector> res;
    res.reserve(ps.size());
    for (auto&& p : ps) {
        res.push_back(
            ir::Projector{
                .alias_field_id = p.alias_field_id,
                .expr =
                    box(ir::Scalar{
                        .node = ir::FieldScalar{.field_id = p.alias_field_id},
                        .value_type = ctx.binding()->type(p.alias_field_id),
                    }),
            });
    }
    return res;
}

void addAllAsFieldScalars(const FieldSet& from, std::vector<ir::Projector>& to, Context& ctx) {
    auto set = schemaFor(to);

    for (auto id : from.fieldIds()) {
        if (set.contains(id)) {
            // already projected
            continue;
        }

        set.append(id);
        to.push_back(
            ir::Projector{
                .alias_field_id = id,
                .expr =
                    box(ir::Scalar{
                        .node = ir::FieldScalar{.field_id = id},
                        .value_type = ctx.binding()->type(id),
                    }),
            });
    }
}

ir::Relation lowerToIR(bound::SelectRelation r, auto& /*info*/, Context& ctx) {
    auto scope = ctx.scopedRelation(lowerToIR(std::move(*r.source), ctx));

    auto source_schema = ctx.currRelation().schema;
    auto visible_fields = source_schema.fieldSet();
    auto _ = ctx.scopedFieldSet(&visible_fields);

    auto [projectors, proj_aggregates] = lowerToIR(std::move(r.projectors), ctx);
    auto projectors_output_fields = schemaFor(projectors);
    auto proj_aggregates_output_fields = schemaFor(proj_aggregates);

    // Update relation because it could've changed after lowering projectors (e.g. markJoin)
    source_schema = ctx.currRelation().schema;
    visible_fields = source_schema.fieldSet();

    if (r.where) {
        util::match(
            r.where->condition->node,
            [&](bound::InExpr& e) {
                auto match = lowerToIR(std::move(*e.match), ctx);
                verify(
                    match.schema.contains(e.match_field_id),
                    "unknown identifier {}",
                    to_string(e.match_field_id, *ctx.binding()));

                auto [key, key_aggregates] = lowerToIR(std::move(*e.expr), ctx);
                verify(key_aggregates.empty());

                ctx.setRelation({
                    .node =
                        ir::SemiJoinRelation{
                            .source = box(ctx.pullRelation()),
                            .match = box(std::move(match)),
                            .expr = box(std::move(key)),
                            .match_field_id = e.match_field_id,
                        },
                    .schema = source_schema,
                });
            },
            [&](const auto&) {
                auto [cond, cond_aggregates] = lowerToIR(std::move(*r.where->condition), ctx);
                verify(cond_aggregates.empty());

                ctx.setRelation({
                    .node =
                        ir::FilterRelation{
                            .source = box(ctx.pullRelation()),
                            .condition = box(std::move(cond)),
                        },
                    .schema = source_schema,
                });
            });
    }

    if (r.group_by.has_value()) {
        auto [group_key, group_key_aggregates] = lowerToIR(std::move(r.group_by->group_list), ctx);
        verify(group_key_aggregates.empty());

        auto group_key_output_fields = schemaFor(group_key);
        auto group_rel = ir::Relation{
            .node =
                ir::GroupRelation{
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(proj_aggregates),
                    .group_list = std::move(group_key),
                },
            .schema = Schema::concat(proj_aggregates_output_fields, group_key_output_fields),
        };

        visible_fields = FieldSet::merge(
            projectors_output_fields.fieldSet(), group_key_output_fields.fieldSet());

        if (r.order_by) {
            auto [order_list, order_aggregates] = lowerToIR(std::move(r.order_by->order_list), ctx);
            verify(order_aggregates.empty());

            auto final_projectors = identityProjectorsFor(projectors, ctx);

            // Forcibly add all group keys that are required by ORDER BY to projectors
            auto required_group_keys = FieldSet::intersection(
                group_key_output_fields.fieldSet(), referencedFieldIdsBy(order_list));
            addAllAsFieldScalars(required_group_keys, projectors, ctx);

            auto projection_fields_out = schemaFor(projectors);
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(std::move(group_rel)),
                        .projectors = std::move(projectors),
                    },
                .schema = projection_fields_out,
            });
            ctx.setRelation({
                .node =
                    ir::SortRelation{
                        .source = box(ctx.pullRelation()),
                        .order_list = std::move(order_list),
                        .desc = r.order_by->desc,
                    },
                .schema = projection_fields_out,
            });
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(ctx.pullRelation()),
                        .projectors = std::move(final_projectors),
                    },
                .schema = projectors_output_fields,
            });
        } else {
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(std::move(group_rel)),
                        .projectors = std::move(projectors),
                    },
                .schema = projectors_output_fields,
            });
        }
    } else if (r.aggregate) {
        verify(!proj_aggregates.empty());
        verify(!r.order_by);

        auto aggregate = ir::Relation{
            .node =
                ir::AggregateRelation{
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(proj_aggregates),
                },
            .schema = proj_aggregates_output_fields,
        };

        ctx.setRelation({
            .node =
                ir::ProjectionRelation{
                    .source = box(std::move(aggregate)),
                    .projectors = std::move(projectors),
                },
            .schema = projectors_output_fields,
        });

        // Old visible fields are dropped
        visible_fields = projectors_output_fields.fieldSet();
    } else {
        verify(proj_aggregates.empty());

        if (r.order_by) {
            // Append to current visible fields
            visible_fields.merge(projectors_output_fields.fieldSet());
            auto [order_list, order_aggregates] = lowerToIR(std::move(r.order_by->order_list), ctx);
            verify(order_aggregates.empty());

            auto final_projectors = identityProjectorsFor(projectors, ctx);

            // Forcibly add all source keys that are required by ORDER BY to projectors
            auto required_source_keys = FieldSet::intersection(
                ctx.currRelation().schema.fieldSet(), referencedFieldIdsBy(order_list));
            addAllAsFieldScalars(required_source_keys, projectors, ctx);

            auto projection_fields_out = schemaFor(projectors);
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(ctx.pullRelation()),
                        .projectors = std::move(projectors),
                    },
                .schema = projection_fields_out,
            });
            ctx.setRelation({
                .node =
                    ir::SortRelation{
                        .source = box(ctx.pullRelation()),
                        .order_list = std::move(order_list),
                        .desc = r.order_by->desc,
                    },
                .schema = projection_fields_out,
            });
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(ctx.pullRelation()),
                        .projectors = std::move(final_projectors),
                    },
                .schema = projectors_output_fields,
            });
        } else {
            ctx.setRelation({
                .node =
                    ir::ProjectionRelation{
                        .source = box(ctx.pullRelation()),
                        .projectors = std::move(projectors),
                    },
                .schema = projectors_output_fields,
            });
        }
    }

    if (r.limit) {
        ctx.setRelation({
            .node =
                ir::LimitRelation{
                    .source = box(ctx.pullRelation()),
                    .limit = r.limit->limit,
                },
            .schema = projectors_output_fields,
        });
    }

    return ctx.pullRelation();
}

ir::Relation lowerToIR(bound::UnionAllRelation r, auto& /*info*/, Context& ctx) {
    auto left = lowerToIR(std::move(*r.left), ctx);
    auto right = lowerToIR(std::move(*r.right), ctx);

    verify(left.schema == right.schema);
    auto schema = left.schema;

    return {
        .node =
            ir::UnionAllRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .schema = schema,
    };
}

ir::Relation lowerToIR(bound::UnionAllSortedByRelation r, auto& /*info*/, Context& ctx) {
    auto left = lowerToIR(std::move(*r.left), ctx);
    auto right = lowerToIR(std::move(*r.right), ctx);

    verify(left.schema == right.schema);
    auto schema = left.schema;
    auto fields = schema.fieldSet();

    auto _ = ctx.scopedFieldSet(&fields);
    auto [order_list, aggregates] = lowerToIR(std::move(r.order_by.order_list), ctx);
    verify(aggregates.empty());

    return {
        .node =
            ir::UnionAllSortedByRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
                .order_list = std::move(order_list),
                .desc = r.order_by.desc,
            },
        .schema = schema,
    };
}

ir::Relation lowerToIR(bound::FileRelation r, auto& info, Context& ctx) {
    auto fields = info.fields_out->fieldSet();
    llog::info("path={}, requested fields: {}", r.path, to_string(fields, *ctx.binding()));

    return {
        .node = ir::FileRelation{.path = std::move(r.path)},
        .schema = Schema::fromFieldSet(fields),
    };
}

ir::Relation lowerToIR(bound::FileIntervalRelation r, auto& info, Context& ctx) {
    auto fields = info.fields_out->fieldSet();
    llog::info(
        "(interval) path={}, requested fields: {}", r.path, to_string(fields, *ctx.binding()));

    return {
        .node =
            ir::FileIntervalRelation{
                .path = std::move(r.path),
                .ts_from = r.ts_from,
                .ts_to = r.ts_to,
            },
        .schema = Schema::fromFieldSet(fields),
    };
}

ir::Relation lowerToIR(bound::NamedRelationReferenceRelation r, auto& /*info*/, Context& ctx) {
    auto schema = ctx.find(r.name);

    return {
        .node = ir::NamedRelationReferenceRelation{.name = std::move(r.name)},
        .schema = schema,
    };
}

ir::Relation lowerToIR(bound::MaterializeRelation r, auto& /*info*/, Context& ctx) {
    auto arg = lowerToIR(std::move(*r.relation), ctx);
    auto schema = arg.schema;

    return {
        .node = ir::MaterializeRelation{.source = box(std::move(arg))},
        .schema = schema,
    };
}

}  // namespace

ir::Relation lowerToIR(bound::Relation expr, Context& ctx) {
    return util::match(
        std::move(expr.node), [&](auto node) { return lowerToIR(std::move(node), expr, ctx); });
}

}  // namespace lsql::front::sql::lower

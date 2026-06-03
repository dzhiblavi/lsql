#include "front/sql/lower/Relations.h"

#include "front/sql/lower/Expressions.h"

#include "ir/tools.h"

#include "util/containers.h"

namespace lsql::front::sql::lower {

namespace {

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

ir::Relation lowerToIR(bound::AdhocRelation r, auto& info, Context& /*ctx*/) {
    return {
        .node =
            ir::ValuesRelation{
                .values = std::move(r.values),
                .output_id = r.output_field_id,
            },
        .fields_out = info.fields_out->fieldSet(),
    };
}

ir::Relation lowerToIR(bound::SelectRelation r, auto& /*info*/, Context& ctx) {
    auto scope = ctx.scopedRelation(lowerToIR(std::move(*r.source), ctx));

    auto visible_fields = ctx.currRelation().fields_out;
    auto _ = ctx.scopedFieldSet(&visible_fields);

    auto [projectors, proj_aggregates] = lowerToIR(std::move(r.projectors), ctx);
    auto projectors_output_fields = outputFieldsOf(projectors);
    auto proj_aggregates_output_fields = outputFieldsOf(proj_aggregates);

    if (r.where) {
        util::match(
            r.where->condition->node,
            [&](bound::InExpr& e) {
                auto match = lowerToIR(std::move(*e.match), ctx);
                verify(
                    match.fields_out.contains(e.match_field_id),
                    "unknown field {}",
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
                    .fields_out = visible_fields,
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
                    .fields_out = visible_fields,
                });
            });
    }

    if (r.group_by.has_value()) {
        auto [group_key, group_key_aggregates] = lowerToIR(std::move(r.group_by->group_list), ctx);
        verify(group_key_aggregates.empty());

        auto group_key_output_fields = outputFieldsOf(group_key);

        auto group_rel = ir::Relation{
            .node =
                ir::GroupRelation{
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(proj_aggregates),
                    .group_list = std::move(group_key),
                },
            .fields_out = FieldSet::merge(proj_aggregates_output_fields, group_key_output_fields),
        };

        ctx.setRelation({
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
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(proj_aggregates),
                },
            .fields_out = proj_aggregates_output_fields,
        };

        ctx.setRelation({
            .node =
                ir::ProjectionRelation{
                    .source = box(std::move(aggregate)),
                    .projectors = std::move(projectors),
                },
            .fields_out = projectors_output_fields,
        });

        // Old visible fields are dropped
        visible_fields = projectors_output_fields;
    } else {
        verify(proj_aggregates.empty());

        ctx.setRelation({
            .node =
                ir::ProjectionRelation{
                    .source = box(ctx.pullRelation()),
                    .projectors = std::move(projectors),
                },
            .fields_out = projectors_output_fields,
        });

        // Append to current visible fields
        visible_fields.merge(projectors_output_fields);
    }

    if (r.order_by) {
        auto [order_list, order_aggregates] = lowerToIR(std::move(r.order_by->order_list), ctx);
        verify(order_aggregates.empty());

        ctx.setRelation({
            .node =
                ir::SortRelation{
                    .source = box(ctx.pullRelation()),
                    .order_list = std::move(order_list),
                    .desc = r.order_by->desc,
                },
            .fields_out = projectors_output_fields,
        });
    }

    if (r.limit) {
        ctx.setRelation({
            .node =
                ir::LimitRelation{
                    .source = box(ctx.pullRelation()),
                    .limit = r.limit->limit,
                },
            .fields_out = projectors_output_fields,
        });
    }

    return ctx.pullRelation();
}

ir::Relation lowerToIR(bound::UnionAllRelation r, auto& /*info*/, Context& ctx) {
    auto left = lowerToIR(std::move(*r.left), ctx);
    auto right = lowerToIR(std::move(*r.right), ctx);
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

ir::Relation lowerToIR(bound::UnionAllSortedByRelation r, auto& /*info*/, Context& ctx) {
    auto left = lowerToIR(std::move(*r.left), ctx);
    auto right = lowerToIR(std::move(*r.right), ctx);
    auto fields = FieldSet::merge(left.fields_out, right.fields_out);

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
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::FileRelation r, auto& info, Context& ctx) {
    auto fields = info.fields_out->fieldSet();
    llog::info("path={}, requested fields: {}", r.path, to_string(fields, *ctx.binding()));

    return {
        .node = ir::FileRelation{.path = std::move(r.path)},
        .fields_out = fields,
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
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::NamedRelationReferenceRelation r, auto& /*info*/, Context& ctx) {
    auto fields = ctx.findRelation(r.name);

    return {
        .node = ir::NamedRelationReferenceRelation{.name = std::move(r.name)},
        .fields_out = fields,
    };
}

ir::Relation lowerToIR(bound::MaterializeRelation r, auto& /*info*/, Context& ctx) {
    auto arg = lowerToIR(std::move(*r.relation), ctx);
    auto fields = arg.fields_out;

    return {
        .node = ir::MaterializeRelation{.source = box(std::move(arg))},
        .fields_out = fields,
    };
}

}  // namespace

ir::Relation lowerToIR(bound::Relation expr, Context& ctx) {
    return util::match(
        std::move(expr.node), [&](auto node) { return lowerToIR(std::move(node), expr, ctx); });
}

}  // namespace lsql::front::sql::lower

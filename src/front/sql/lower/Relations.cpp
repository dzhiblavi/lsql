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
    const FieldSet& star_fields,
    Context& ctx) {
    util::match(
        std::move(p),
        [&](bound::StarProjector) {
            for (auto id : star_fields.fieldIds()) {
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
    std::vector<bound::Projector> projectors, const FieldSet& star_fields, Context& ctx) {
    std::vector<ir::Projector> exprs;
    std::vector<ir::Aggregate> aggrs;
    exprs.reserve(projectors.size());
    for (auto&& p : projectors) {
        lowerToIR(std::move(p), exprs, aggrs, star_fields, ctx);
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

void setProjection(std::vector<ir::Projector> projectors, Context& ctx) {
    auto schema = schemaFor(projectors);
    ctx.setRelation({
        .node =
            ir::ProjectionRelation{
                .source = box(ctx.pullRelation()),
                .projectors = std::move(projectors),
            },
        .schema = std::move(schema),
    });
}

void lowerWhere(bound::Where where, Context& ctx) {
    auto schema = ctx.currRelation().schema;
    auto fields = schema.fieldSet();
    auto field_scope = ctx.scopedFieldSet(&fields);

    if (auto* in = std::get_if<bound::InExpr>(&where.condition->node)) {
        auto match = lowerToIR(std::move(*in->match), ctx);
        verify(match.schema.contains(in->match_field_id));

        auto [key, aggregates] = lowerToIR(std::move(*in->expr), ctx);
        verify(aggregates.empty());

        ctx.setRelation({
            .node =
                ir::SemiJoinRelation{
                    .source = box(ctx.pullRelation()),
                    .match = box(std::move(match)),
                    .expr = box(std::move(key)),
                    .match_field_id = in->match_field_id,
                },
            .schema = std::move(schema),
        });
    } else {
        auto [condition, aggregates] = lowerToIR(std::move(*where.condition), ctx);
        verify(aggregates.empty());

        ctx.setRelation({
            .node =
                ir::FilterRelation{
                    .source = box(ctx.pullRelation()),
                    .condition = box(std::move(condition)),
                },
            .schema = std::move(schema),
        });
    }
}

std::vector<ir::Projector> prepareGroupKeys(bound::GroupBy group_by, Context& ctx) {
    auto fields = ctx.currRelation().schema.fieldSet();
    auto field_scope = ctx.scopedFieldSet(&fields);
    auto [keys, aggregates] = lowerToIR(std::move(group_by.group_list), fields, ctx);
    verify(aggregates.empty());

    auto group_keys = identityProjectorsFor(keys, ctx);
    addAllAsFieldScalars(ctx.currRelation().schema.fieldSet(), keys, ctx);
    setProjection(std::move(keys), ctx);
    return group_keys;
}

void lowerSelectOutput(
    std::vector<ir::Projector> projectors,
    std::optional<bound::Where> having,
    std::optional<bound::OrderBy> order_by,
    Context& ctx) {
    auto final_projectors = identityProjectorsFor(projectors, ctx);
    auto required_fields = FieldSet::emptySet();

    if (having) {
        required_fields.merge(having->condition->required_fields);
    }

    if (order_by) {
        for (auto& expression : order_by->order_list) {
            required_fields.merge(expression.required_fields);
        }
    }

    addAllAsFieldScalars(required_fields, projectors, ctx);
    setProjection(std::move(projectors), ctx);

    if (having) {
        lowerWhere(std::move(*having), ctx);
    }

    if (order_by) {
        auto fields = ctx.currRelation().schema.fieldSet();
        auto field_scope = ctx.scopedFieldSet(&fields);
        auto [order_list, aggregates] = lowerToIR(std::move(order_by->order_list), ctx);
        verify(aggregates.empty());

        auto schema = ctx.currRelation().schema;
        ctx.setRelation({
            .node =
                ir::SortRelation{
                    .source = box(ctx.pullRelation()),
                    .order_list = std::move(order_list),
                    .desc = order_by->desc,
                },
            .schema = std::move(schema),
        });
    }

    if (having || order_by) {
        setProjection(std::move(final_projectors), ctx);
    }
}

void lowerGroupedSelect(bound::SelectRelation& select, Context& ctx) {
    auto group_keys = prepareGroupKeys(std::move(*select.group_by), ctx);
    auto group_schema = schemaFor(group_keys);
    auto input_fields = ctx.currRelation().schema.fieldSet();
    auto input_scope = ctx.scopedFieldSet(&input_fields);

    auto [projectors, aggregates] =
        lowerToIR(std::move(select.projectors), group_schema.fieldSet(), ctx);

    if (select.having) {
        util::append(aggregates, extractAggregates(*select.having->condition, ctx));
    }

    auto schema = Schema::concat(schemaFor(aggregates), group_schema);
    ctx.setRelation({
        .node =
            ir::GroupRelation{
                .source = box(ctx.pullRelation()),
                .aggregates = std::move(aggregates),
                .group_list = std::move(group_keys),
            },
        .schema = std::move(schema),
    });

    lowerSelectOutput(
        std::move(projectors), std::move(select.having), std::move(select.order_by), ctx);
}

void lowerUngroupedSelect(
    bound::SelectRelation& select, const FieldSet& source_fields, Context& ctx) {
    auto fields = ctx.currRelation().schema.fieldSet();
    auto field_scope = ctx.scopedFieldSet(&fields);
    auto [projectors, aggregates] = lowerToIR(std::move(select.projectors), source_fields, ctx);

    if (select.aggregate) {
        verify(!select.order_by);
        verify(!aggregates.empty());
        auto schema = schemaFor(aggregates);
        ctx.setRelation({
            .node =
                ir::AggregateRelation{
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(aggregates),
                },
            .schema = std::move(schema),
        });
    } else {
        verify(aggregates.empty());
    }

    lowerSelectOutput(std::move(projectors), std::nullopt, std::move(select.order_by), ctx);
}

ir::Relation lowerToIR(bound::SelectRelation select, auto&, Context& ctx) {
    auto relation_scope = ctx.scopedRelation(lowerToIR(std::move(*select.source), ctx));
    auto source_fields = ctx.currRelation().schema.fieldSet();

    if (select.where) {
        lowerWhere(std::move(*select.where), ctx);
    }

    if (select.group_by) {
        lowerGroupedSelect(select, ctx);
    } else {
        lowerUngroupedSelect(select, source_fields, ctx);
    }

    if (select.limit) {
        auto schema = ctx.currRelation().schema;
        ctx.setRelation({
            .node =
                ir::LimitRelation{
                    .source = box(ctx.pullRelation()),
                    .limit = select.limit->limit,
                },
            .schema = std::move(schema),
        });
    }

    return ctx.pullRelation();
}

ir::Relation lowerToIR(bound::AliasRelation r, auto&, Context& ctx) {
    auto source = lowerToIR(std::move(*r.source), ctx);

    std::vector<ir::Projector> projectors;
    for (auto id : source.schema.fieldIds()) {
        auto name = std::format("{}.{}", r.alias, ctx.binding()->name(id));
        auto type = ctx.binding()->type(id);

        projectors.push_back({
            .alias_field_id = ctx.binding()->getOrAdd(name, type),
            .expr =
                box(ir::Scalar{
                    .node = ir::FieldScalar{.field_id = id},
                    .value_type = type,
                }),
        });
    }

    auto schema = schemaFor(projectors);

    return {
        .node =
            ir::ProjectionRelation{
                .source = box(std::move(source)),
                .projectors = std::move(projectors),
            },
        .schema = schema,
    };
}

ir::Relation lowerToIR(bound::LeftJoinRelation r, auto&, Context& ctx) {
    auto lower_input = [&](bound::Relation source, bound::Expr expr) {
        auto relation_scope = ctx.scopedRelation(lowerToIR(std::move(source), ctx));
        auto output_schema = ctx.currRelation().schema;
        auto fields = output_schema.fieldSet();
        auto fields_scope = ctx.scopedFieldSet(&fields);
        auto [key, aggregates] = lowerToIR(std::move(expr), ctx);
        verify(aggregates.empty());
        return std::tuple{ctx.pullRelation(), std::move(key), std::move(output_schema)};
    };

    // clang-format off
    auto [left, left_key, left_output] = lower_input(std::move(*r.left), std::move(*r.left_key));
    auto [right, right_key, right_output] = lower_input(std::move(*r.right), std::move(*r.right_key));
    // clang-format on

    auto schema = Schema::concat(left.schema, right.schema);
    auto output_schema = Schema::concat(left_output, right_output);

    auto joined = ir::Relation{
        .node =
            ir::LeftJoinRelation{
                .left = box(std::move(left)),
                .right = box(std::move(right)),
                .left_key = box(std::move(left_key)),
                .right_key = box(std::move(right_key)),
            },
        .schema = std::move(schema),
    };
    if (joined.schema == output_schema) {
        return joined;
    }

    std::vector<ir::Projector> projectors;
    for (auto id : output_schema.fieldIds()) {
        projectors.push_back({
            .alias_field_id = id,
            .expr =
                box(ir::Scalar{
                    .node = ir::FieldScalar{.field_id = id},
                    .value_type = ctx.binding()->type(id),
                }),
        });
    }

    return {
        .node =
            ir::ProjectionRelation{
                .source = box(std::move(joined)),
                .projectors = std::move(projectors),
            },
        .schema = std::move(output_schema),
    };
}

ir::Relation lowerToIR(bound::UnionAllRelation r, auto& /*info*/, Context& ctx) {
    auto left = lowerToIR(std::move(*r.left), ctx);
    auto right = lowerToIR(std::move(*r.right), ctx);

    verify(left.schema == right.schema);
    auto schema = left.schema;

    std::vector<ir::Relation> rels;
    rels.push_back(std::move(left));
    rels.push_back(std::move(right));

    return {
        .node = ir::UnionAllRelation{.sources = std::move(rels)},
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
    auto fields = info.fields_out->fields();
    llog::trace("path={}, requested fields: {}", r.path, to_string(fields, *ctx.binding()));

    return {
        .node = ir::FileRelation{.path = std::move(r.path)},
        .schema = Schema::fromFieldSet(fields),
    };
}

ir::Relation lowerToIR(bound::FileIntervalRelation r, auto& info, Context& ctx) {
    auto fields = info.fields_out->fields();
    llog::trace(
        "(interval) path={}, requested fields: {}", r.path, to_string(fields, *ctx.binding()));

    return {
        .node =
            ir::FileIntervalRelation{
                .path = std::move(r.path),
                .ts_from = r.ts_from,
                .ts_to = r.ts_to,
                .time_format = r.time_format,
            },
        .schema = Schema::fromFieldSet(fields),
    };
}

ir::Relation lowerToIR(bound::StreamRelation r, auto& info, Context& ctx) {
    auto fields = info.fields_out->fields();
    llog::trace(
        "stream command={}, requested fields: {}", r.command, to_string(fields, *ctx.binding()));

    return {
        .node = ir::StreamRelation{.command = std::move(r.command)},
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

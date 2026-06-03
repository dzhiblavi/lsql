#include "front/pipe/lower/Stages.h"
#include "front/pipe/lower/Expressions.h"
#include "front/pipe/lower/Pipeline.h"

#include "front/pipe/bound/Expressions.h"  // IWYU pragma: keep
#include "front/pipe/bound/Pipeline.h"     // IWYU pragma: keep
#include "front/pipe/bound/Sources.h"      // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"

#include "ir/tools.h"

#include "util/containers.h"

namespace lsql::front::pipe::lower {

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

ir::Relation lowerToIR(bound::FilterStage s, bound::Stage& /*self*/, Context& ctx) {
    auto [cond, aggregates] = lowerToIR(std::move(*s.condition), ctx);
    verify(aggregates.empty());
    auto fields_out = ctx.currRelation().fields_out;

    return ir::Relation{
        .node =
            ir::FilterRelation{
                .source = box(ctx.pullRelation()),
                .condition = box(std::move(cond)),
            },
        .fields_out = fields_out,
    };
}

ir::Relation lowerToIR(bound::WhereInStage s, auto& /*self*/, Context& ctx) {
    auto fields_out = ctx.currRelation().fields_out;

    // lower match against a separate context
    auto match_ctx = ctx.subContext();
    auto match = lower::lowerToIR(std::move(*s.match), match_ctx);
    verify(
        match.fields_out.contains(s.match_field_id),
        "unknown field {}",
        to_string(s.match_field_id, *ctx.binding()));

    auto [key, key_aggregates] = lowerToIR(std::move(*s.expr), ctx);
    verify(key_aggregates.empty());

    return {
        .node =
            ir::SemiJoinRelation{
                .source = box(ctx.pullRelation()),
                .match = box(std::move(match)),
                .expr = box(std::move(key)),
                .match_field_id = s.match_field_id,
            },
        .fields_out = fields_out,
    };
}

ir::Relation lowerToIR(bound::TakeStage s, auto& /*self*/, Context& ctx) {
    auto source = box(ctx.pullRelation());
    auto fields_out = source->fields_out;

    return ir::Relation{
        .node =
            ir::LimitRelation{
                .source = std::move(source),
                .limit = s.count,
            },
        .fields_out = fields_out,
    };
}

ir::Relation lowerToIR(bound::SortStage s, auto& /*self*/, Context& ctx) {
    auto [order_list, order_aggregates] = lowerToIR(std::move(s.order_list), ctx);
    verify(order_aggregates.empty());

    auto source = box(ctx.pullRelation());
    auto fields_out = source->fields_out;

    return ir::Relation{
        .node =
            ir::SortRelation{
                .source = std::move(source),
                .order_list = std::move(order_list),
                .desc = s.desc,
            },
        .fields_out = fields_out,
    };
}

ir::Relation lowerToIR(bound::SelectStage s, auto& /*self*/, Context& ctx) {
    auto [scalars, aggregates] = lowerToIR(std::move(s.projectors), ctx);
    auto projectors_output_fields = outputFieldsOf(scalars);
    auto proj_aggregates_output_fields = outputFieldsOf(aggregates);

    if (aggregates.empty()) {
        return {
            .node =
                ir::ProjectionRelation{
                    .source = box(ctx.pullRelation()),
                    .projectors = std::move(scalars),
                },
            .fields_out = projectors_output_fields,
        };
    } else {
        auto aggregate = ir::Relation{
            .node =
                ir::AggregateRelation{
                    .source = box(ctx.pullRelation()),
                    .aggregates = std::move(aggregates),
                },
            .fields_out = proj_aggregates_output_fields,
        };

        return {
            .node =
                ir::ProjectionRelation{
                    .source = box(std::move(aggregate)),
                    .projectors = std::move(scalars),
                },
            .fields_out = projectors_output_fields,
        };
    }
}

ir::Relation lowerToIR(bound::GroupStage s, auto& /*self*/, Context& ctx) {
    // lowered against ctx.currRelation()
    auto [group_list_scalar, group_list_aggregates] = lowerToIR(std::move(s.group_list), ctx);
    verify(group_list_aggregates.empty());
    auto group_list_scalar_fields = outputFieldsOf(group_list_scalar);

    auto [proj_scalars, proj_aggregates] = lowerToIR(std::move(s.projectors), ctx);
    auto proj_aggregates_fields = outputFieldsOf(proj_aggregates);

    auto group = ir::GroupRelation{
        .source = box(ctx.pullRelation()),
        .aggregates = std::move(proj_aggregates),
        .group_list = std::move(group_list_scalar),
    };

    auto group_fields_out = FieldSet::merge(proj_aggregates_fields, group_list_scalar_fields);
    auto fields_out = outputFieldsOf(proj_scalars);

    return {
        .node =
            ir::ProjectionRelation{
                .source =
                    box(ir::Relation{
                        .node = std::move(group),
                        .fields_out = group_fields_out,
                    }),
                .projectors = std::move(proj_scalars),
            },
        .fields_out = fields_out,
    };
}

}  // namespace

ir::Relation lowerToIR(bound::Stage s, Context& ctx) {
    return util::match(std::move(s.node), [&](auto r) { return lowerToIR(std::move(r), s, ctx); });
}

}  // namespace lsql::front::pipe::lower

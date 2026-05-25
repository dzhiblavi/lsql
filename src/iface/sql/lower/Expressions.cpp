#include "iface/sql/lower/Expressions.h"

#include "iface/sql/lower/Relations.h"
#include "iface/sql/lower/helpers.h"

namespace lsql::iface::sql::lower {

namespace {

LowerExprResult lowerToIR(bound::IdentifierExpr e, auto& info, Context& /*ctx*/) {
    return LowerExprResult(
        {
            .node = ir::FieldScalar{.field_id = e.field_id},
            .value_type = info.value_type,
        },
        {});
}

LowerExprResult lowerToIR(bound::ValueExpr e, auto& info, Context& /*ctx*/) {
    return LowerExprResult(
        {
            .node = ir::ValueScalar{.value = std::move(e.value)},
            .value_type = info.value_type,
        },
        {});
}

LowerExprResult lowerToIR(bound::CastExpr e, auto& info, Context& ctx) {
    auto [arg, aggregates] = lowerToIR(std::move(*e.expr), ctx);

    return LowerExprResult(
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

LowerExprResult lowerToIR(bound::InExpr e, auto& /*info*/, Context& ctx) {
    auto output_id = ctx.binding()->addAnonymous("mark_join", ValueType::Boolean);
    auto [expr, aggregates] = lowerToIR(std::move(*e.expr), ctx);
    verify(aggregates.empty());
    auto match = lowerToIR(std::move(*e.match), ctx);

    auto source = ctx.pullRelation();
    auto fields = source.fields_out;
    fields.add(output_id);

    // same source relation enriched with a boolean field indicating
    // whether the row's key matches `match`
    ctx.setRelation({
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

    return LowerExprResult(
        {
            .node = ir::FieldScalar{.field_id = output_id},
            .value_type = ValueType::Boolean,
        },
        {});
}

LowerExprResult lowerToIR(bound::LikeExpr e, auto& info, Context& ctx) {
    auto [arg, aggregates] = lowerToIR(std::move(*e.expr), ctx);

    return LowerExprResult(
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

LowerExprResult lowerToIR(bound::UnaryAggregateExpr e, auto& info, Context& ctx) {
    auto output_field_id = ctx.binding()->addAnonymous("un_agr", info.value_type);
    auto [expr, aggregates] = lowerToIR(std::move(*e.expr), ctx);
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

    return LowerExprResult(
        {
            .node = ir::FieldScalar{.field_id = output_field_id},
            .value_type = info.value_type,
        },
        std::move(aggrs));
}

LowerExprResult lowerToIR(bound::RSubstrExpr e, auto& info, Context& ctx) {
    auto [expr, aggregates] = lowerToIR(std::move(*e.expr), ctx);

    return LowerExprResult(
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

LowerExprResult lowerToIR(bound::CoalesceExpr e, auto& info, Context& ctx) {
    auto [exprs, aggregates] = lowerToIR(std::move(e.args), ctx);

    return LowerExprResult(
        {
            .node = ir::CoalesceScalar{.args = std::move(exprs)},
            .value_type = info.value_type,
        },
        std::move(aggregates));
}

LowerExprResult lowerToIR(bound::PercentileExpr e, auto& info, Context& ctx) {
    auto output_field_id = ctx.binding()->addAnonymous("perc", ValueType::String);
    auto [expr, aggregates] = lowerToIR(std::move(*e.expr), ctx);
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

    return LowerExprResult(
        ir::Scalar{
            .node = ir::FieldScalar{.field_id = output_field_id},
            .value_type = ValueType::String,
        },
        std::move(aggrs));
}

LowerExprResult lowerToIR(bound::BinaryExpr e, auto& info, Context& ctx) {
    auto [left, al] = lowerToIR(std::move(*e.left), ctx);
    auto [right, ar] = lowerToIR(std::move(*e.right), ctx);

    return LowerExprResult(
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

LowerExprResult lowerToIR(bound::UnaryExpr e, auto& info, Context& ctx) {
    auto [arg, aggregates] = lowerToIR(std::move(*e.expr), ctx);

    return LowerExprResult(
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

}  // namespace

LowerExprsResult lowerToIR(std::vector<bound::Expr> es, Context& ctx) {
    std::vector<ir::Scalar> exprs;
    exprs.reserve(es.size());
    std::vector<ir::Aggregate> aggrs;

    for (auto&& p : es) {
        auto [expr, aggregates] = lowerToIR(std::move(p), ctx);
        exprs.push_back(std::move(expr));
        append(aggrs, std::move(aggregates));
    }

    return std::make_pair(std::move(exprs), std::move(aggrs));
}

LowerExprResult lowerToIR(bound::Expr expr, Context& ctx) {
    return util::match(
        std::move(expr.node), [&](auto node) { return lowerToIR(std::move(node), expr, ctx); });
}

}  // namespace lsql::iface::sql::lower

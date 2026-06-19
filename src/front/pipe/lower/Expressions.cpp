#include "front/pipe/lower/Expressions.h"

#include "front/common/lower/Functions.h"
#include "front/pipe/lower/Pipeline.h"

#include "front/pipe/bound/Sources.h"  // IWYU pragma: keep
#include "front/pipe/bound/Stages.h"   // IWYU pragma: keep

#include "util/containers.h"

namespace lsql::front::pipe::lower {

namespace {

LowerExprResult lowerToIR(bound::IdentifierExpr e, auto& info, Context& ctx) {
    verify(
        ctx.currFieldSet().contains(e.field_id),
        "unknown identifier: {}",
        to_string(e.field_id, *ctx.binding()));

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

LowerExprResult lowerToIR(bound::InExpr e, auto& /*info*/, Context& ctx) {
    auto output_id = ctx.binding()->addAnonymous("mark_join", ValueType::Boolean);

    // lower against ctx.currFieldSet()
    auto [expr, aggregates] = lowerToIR(std::move(*e.expr), ctx);
    verify(aggregates.empty());

    // lower against a separate context
    auto match_ctx = ctx.subContext();
    auto match = lower::lowerToIR(std::move(*e.match), match_ctx);
    verify(
        match.schema.contains(e.match_field_id),
        "unknown identifier {}",
        to_string(e.match_field_id, *ctx.binding()));

    auto source = ctx.pullRelation();
    auto schema = source.schema;
    schema.append(output_id);

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
        .schema = schema,
    });

    return LowerExprResult(
        {
            .node = ir::FieldScalar{.field_id = output_id},
            .value_type = ValueType::Boolean,
        },
        {});
}

LowerExprResult lowerToIR(bound::FnCallExpr e, auto& info, Context& ctx) {
    auto [scalars, aggregates] = lowerToIR(std::move(e.args), ctx);
    verify(aggregates.empty());

    if (func::isScalar(e.func)) {
        return LowerExprResult(
            {
                .node =
                    ir::FnCallScalar{
                        .function = e.func,
                        .args = std::move(scalars),
                    },
                .value_type = info.value_type,
            },
            std::move(aggregates));
    }

    auto output_field_id = ctx.binding()->addAnonymous("fn_call_aggr", info.value_type);
    aggregates.push_back({
        .node =
            ir::FnCallAggregate{
                .function = e.func,
                .args = std::move(scalars),
            },
        .output_field_id = output_field_id,
        .value_type = info.value_type,
    });

    return LowerExprResult{
        {
            .node = ir::FieldScalar{.field_id = output_field_id},
            .value_type = info.value_type,
        },
        std::move(aggregates),
    };
}

LowerExprResult lowerToIR(bound::LikeExpr e, auto& info, Context& ctx) {
    auto [arg, aggregates] = lowerToIR(std::move(*e.expr), ctx);

    std::vector<ir::Scalar> args;
    args.push_back(std::move(arg));

    return LowerExprResult(
        {
            .node =
                ir::FnCallScalar{
                    .function = func::Like{.regex = std::move(e.regex)},
                    .args = std::move(args),
                },
            .value_type = info.value_type,
        },
        std::move(aggregates));
}

LowerExprResult lowerToIR(bound::BinaryExpr e, auto& info, Context& ctx) {
    auto [left, al] = lowerToIR(std::move(*e.left), ctx);
    auto [right, ar] = lowerToIR(std::move(*e.right), ctx);

    std::vector<ir::Scalar> args;
    args.push_back(std::move(left));
    args.push_back(std::move(right));

    return LowerExprResult(
        {
            .node =
                ir::FnCallScalar{
                    .function = common::lower::function(e.type, info.value_type),
                    .args = std::move(args),
                },
            .value_type = info.value_type,
        },
        util::concat(std::move(al), std::move(ar)));
}

LowerExprResult lowerToIR(bound::UnaryExpr e, auto& info, Context& ctx) {
    auto [arg, aggregates] = lowerToIR(std::move(*e.expr), ctx);
    std::vector<ir::Scalar> args;
    args.push_back(std::move(arg));

    return LowerExprResult(
        {
            .node =
                ir::FnCallScalar{
                    .function = common::lower::function(e.type),
                    .args = std::move(args),
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
        util::append(aggrs, std::move(aggregates));
    }

    return std::make_pair(std::move(exprs), std::move(aggrs));
}

LowerExprResult lowerToIR(bound::Expr expr, Context& ctx) {
    return util::match(
        std::move(expr.node), [&](auto node) { return lowerToIR(std::move(node), expr, ctx); });
}

}  // namespace lsql::front::pipe::lower

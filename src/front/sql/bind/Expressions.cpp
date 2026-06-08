#include "front/sql/bind/Expressions.h"

#include "front/sql/bind/Context.h"
#include "front/sql/bind/Relations.h"

#include "front/sql/ast/Expressions.h"
#include "front/sql/ast/Relations.h"  // IWYU pragma: keep

#include "front/common/bind/Expressions.h"
#include "front/common/source/require_at.h"

#include "front/sql/bound/Expressions.h"
#include "front/sql/bound/Relations.h"

namespace lsql::front::sql::bind {

std::vector<bound::Expr> bindExprs(std::vector<ast::Expr> exprs, Context& ctx) {
    return common::bind::bindExprs<bound::Expr>(std::move(exprs), bindExpr, ctx);
}

bound::Expr bindExpr(ast::IdentifierExpr e, ast::Expr& self, Context& ctx) {
    auto maybe_type = ctx.currFieldSet().typeOfSourceField(e.identifier, ctx.binding());
    requireAt(maybe_type.has_value(), self.span, "unknown field '{}'", e.identifier);
    auto type = *maybe_type;
    auto id = ctx.binding()->getOrAdd(e.identifier, type);

    return {
        .node = bound::IdentifierExpr{.field_id = id},
        .value_type = type,
        .level = common::bound::ExprKindLevel::Row,
        .required_fields = FieldSet::withField(id),
    };
}

bound::Expr bindExpr(ast::LiteralExpr e, auto&& self, Context& /*ctx*/) {
    auto value = parseLiteral(e.literal);
    requireAt(value.has_value(), self.span, "invalid literal '{}'", e.literal.value_str);

    return {
        .node = bound::ValueExpr{.value = *value},
        .value_type = value->type(),
        .level = common::bound::ExprKindLevel::Const,
        .required_fields = FieldSet::emptySet(),
    };
}

bound::Expr bindExpr(ast::CastExpr e, auto&& /*self*/, Context& ctx) {
    auto expr = bindExpr(std::move(*e.expr), ctx);
    auto info = common::bind::bindCast(expr, e.cast_to);

    return {
        .node =
            bound::CastExpr{
                .cast_to = e.cast_to,
                .expr = box(std::move(expr)),
            },
        .value_type = info.value_type,
        .level = info.level,
        .required_fields = info.required_fields,
    };
}

bound::Expr bindExpr(ast::InExpr e, auto&& /*self*/, Context& ctx) {
    auto expr_span = e.expr->span;
    auto match_span = e.match->span;

    auto expr = bindExpr(std::move(*e.expr), ctx);
    auto match = bindRelation(std::move(*e.match), ctx);
    auto [info, match_field_id] = common::bind::bindInExpr(expr, match, ctx, expr_span, match_span);

    return {
        .node =
            bound::InExpr{
                .expr = box(std::move(expr)),
                .match = box(std::move(match)),
                .match_field_id = match_field_id,
            },
        .value_type = info.value_type,
        .level = info.level,
        .required_fields = info.required_fields,
    };
}

bound::Expr bindExpr(ast::LikeExpr e, auto&& /*self*/, Context& ctx) {
    auto arg_span = e.expr->span;
    auto arg = bindExpr(std::move(*e.expr), ctx);
    auto info = common::bind::bindLikeExpr(arg, arg_span);

    return {
        .node =
            bound::LikeExpr{
                .expr = box(std::move(arg)),
                .regex = std::move(e.regex),
            },
        .value_type = info.value_type,
        .level = info.level,
        .required_fields = info.required_fields,
    };
}

bound::Expr bindExpr(ast::FnCallExpr e, auto&& self, Context& ctx) {
    auto args_span = spanOf(e.args);
    auto args = bindExprs(std::move(e.args), ctx);

    if (auto un_aggr_type = common::bind::unaryAggregateType(e.func)) {
        auto info = common::bind::bindUnaryAggregate(args, *un_aggr_type, args_span);

        return {
            .node =
                bound::UnaryAggregateExpr{
                    .type = *un_aggr_type,
                    .expr = box(std::move(args[0])),
                },
            .value_type = info.value_type,
            .level = info.level,
            .required_fields = info.required_fields,
        };
    }

    if (e.func == "builtin_count_all") {
        auto info = common::bind::bindCountAll(args, args_span);

        return {
            .node = bound::CountAllExpr{},
            .value_type = info.value_type,
            .level = info.level,
            .required_fields = info.required_fields,
        };
    }

    if (e.func == "builtin_coalesce") {
        auto info = common::bind::bindCoalesce(args, args_span);

        return {
            .node = bound::CoalesceExpr{.args = std::move(args)},
            .value_type = info.value_type,
            .level = info.level,
            .required_fields = info.required_fields,
        };
    }

    if (e.func == "builtin_percentile") {
        auto [info, percentiles] = common::bind::bindPercentile(args, args_span);

        return {
            .node =
                bound::PercentileExpr{
                    .expr = box(std::move(args[0])),
                    .percentiles = std::move(percentiles),
                },
            .value_type = info.value_type,
            .level = info.level,
            .required_fields = info.required_fields,
        };
    }

    if (e.func == "builtin_rsubstr") {
        auto [info, regex] = common::bind::bindRsubstr(args, args_span);

        return {
            .node =
                bound::RSubstrExpr{
                    .expr = box(std::move(args[0])),
                    .regex = std::move(regex),
                },
            .value_type = info.value_type,
            .level = info.level,
            .required_fields = info.required_fields,
        };
    }

    throwAt(self.span, "unknown function name: {}", e.func);
}

bound::Expr bindExpr(ast::BinaryExpr e, auto&& self, Context& ctx) {
    auto left = bindExpr(std::move(*e.left), ctx);
    auto right = bindExpr(std::move(*e.right), ctx);
    auto [info, type] = common::bind::bindBinaryExpr(left, right, e.type, self.span);

    return {
        .node =
            bound::BinaryExpr{
                .type = type,
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .value_type = info.value_type,
        .level = info.level,
        .required_fields = info.required_fields,
    };
}

bound::Expr bindExpr(ast::UnaryExpr e, auto&& /*self*/, Context& ctx) {
    auto arg = bindExpr(std::move(*e.expr), ctx);
    auto [info, type] = common::bind::bindUnaryExpr(arg, e.type);

    return {
        .node =
            bound::UnaryExpr{
                .type = type,
                .expr = box(std::move(arg)),
            },
        .value_type = info.value_type,
        .level = info.level,
        .required_fields = info.required_fields,
    };
}

bound::Expr bindExpr(ast::Expr expr, Context& ctx) {
    return util::match(
        std::move(expr.node), [&](auto node) { return bindExpr(std::move(node), expr, ctx); });
}

}  // namespace lsql::front::sql::bind

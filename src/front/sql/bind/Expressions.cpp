#include "front/sql/bind/Expressions.h"

#include "front/sql/bind/Context.h"
#include "front/sql/bind/Relations.h"
#include "front/sql/bind/helpers.h"

#include "front/sql/ast/Expressions.h"
#include "front/sql/ast/Relations.h"  // IWYU pragma: keep

#include "front/Expressions.h"

#include "front/sql/bound/Expressions.h"

#include "core/exprs/BinaryExpr.h"
#include "core/exprs/Percentile.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/exprs/UnaryExpr.h"

#include "util/enum.h"

namespace lsql::front::sql::bind {

namespace {

UnaryExprType exprType(ast::UnaryExprType ast) {
    switch (ast) {
        case ast::UnaryExprType::Not:
            return UnaryExprType::BooleanNegate;
    }
}

BinaryExprType exprType(ast::BinaryExprType ast) {
    switch (ast) {
        case ast::BinaryExprType::Equal:
            return BinaryExprType::Equal;
        case ast::BinaryExprType::NotEqual:
            return BinaryExprType::NotEqual;
        case ast::BinaryExprType::And:
            return BinaryExprType::And;
        case ast::BinaryExprType::Or:
            return BinaryExprType::Or;
        case ast::BinaryExprType::Divide:
            return BinaryExprType::Divide;
        case ast::BinaryExprType::Plus:
            return BinaryExprType::Add;
        case ast::BinaryExprType::Minus:
            return BinaryExprType::Subtract;
    }
}

std::optional<UnaryAggregateType> unaryAggregateType(std::string_view fn_name) {
    static constexpr std::array<std::pair<std::string_view, UnaryAggregateType>, 4> Types{
        std::make_pair("builtin_count_nonnull", UnaryAggregateType::CountNonNull),
        std::make_pair("builtin_min", UnaryAggregateType::Min),
        std::make_pair("builtin_max", UnaryAggregateType::Max),
        std::make_pair("builtin_sum", UnaryAggregateType::Sum),
    };

    auto it = std::ranges::find(Types, fn_name, [](auto&& p) { return p.first; });
    return it == Types.end() ? std::nullopt : std::optional(it->second);
}

ValueType valueType(ValueType arg, UnaryExprType type) {
    return util::enum_dispatch([&]<auto Type>() { return unaryExprResultType<Type>(arg); }, type);
}

ValueType valueType(ValueType l, ValueType r, BinaryExprType type) {
    return util::enum_dispatch([&]<auto Type>() { return binaryExprResultType<Type>(l, r); }, type);
}

ValueType unaryAggregateValueType(UnaryAggregateType type, ValueType arg) {
    return util::enum_dispatch(
        [&]<auto Type>() { return unaryAggregateResultType<Type>(arg); }, type);
}

}  // namespace

std::vector<bound::Expr> bindExprs(std::vector<ast::Expr> exprs, Context& ctx) {
    std::vector<bound::Expr> result;
    result.reserve(exprs.size());
    for (auto&& p : exprs) {
        result.push_back(bindExpr(std::move(p), ctx));
    }
    return result;
}

bound::Expr bindExpr(ast::IdentifierExpr e, Context& ctx) {
    auto type = ctx.currFieldSet().typeOfSourceField(e.identifier, ctx.binding());
    auto id = ctx.binding()->getOrAdd(e.identifier, type);

    return {
        .node = bound::IdentifierExpr{.field_id = id},
        .value_type = type,
        .level = ExprKindLevel::Row,
        .required_fields = FieldSet::withField(id),
    };
}

bound::Expr bindExpr(ast::LiteralExpr e, Context& /*ctx*/) {
    auto value = parseLiteral(e.literal);

    return {
        .node = bound::ValueExpr{.value = value},
        .value_type = value.type(),
        .level = ExprKindLevel::Const,
        .required_fields = FieldSet::emptySet(),
    };
}

bound::Expr bindExpr(ast::CastExpr e, Context& ctx) {
    auto expr = bindExpr(std::move(*e.expr), ctx);
    auto type = e.cast_to;
    auto level = expr.level;
    auto req_fields = expr.required_fields;

    return {
        .node =
            bound::CastExpr{
                .cast_to = e.cast_to,
                .expr = box(std::move(expr)),
            },
        .value_type = type,
        .level = level,
        .required_fields = req_fields,
    };
}

bound::Expr bindExpr(ast::InExpr e, Context& ctx) {
    auto expr = bindExpr(std::move(*e.expr), ctx);
    require(expr.level != ExprKindLevel::Group, "IN key expression cannot be aggregate");
    auto req_fields = expr.required_fields;

    bound::Relation match = bindRelation(std::move(*e.match), ctx);
    auto count = match.fields_out->fieldSet().fieldIds().size();
    verify(count == 1, "expected 1, got {}", count);
    require(count == 1, "IN match should contain one column, got {}", count);

    auto match_field_id = *match.fields_out->fieldSet().fieldIds().begin();

    return {
        .node =
            bound::InExpr{
                .expr = box(std::move(expr)),
                .match = box(std::move(match)),
                .match_field_id = match_field_id,
            },
        .value_type = ValueType::Boolean,
        .level = ExprKindLevel::Row,
        .required_fields = req_fields,
    };
}

bound::Expr bindExpr(ast::LikeExpr e, Context& ctx) {
    auto arg = bindExpr(std::move(*e.expr), ctx);
    require(arg.value_type == ValueType::String, "LIKE argument should be String");
    auto level = arg.level;
    auto req_fields = arg.required_fields;

    return {
        .node =
            bound::LikeExpr{
                .expr = box(std::move(arg)),
                .regex = std::move(e.regex),
            },
        .value_type = ValueType::Boolean,
        .level = level,
        .required_fields = req_fields,
    };
}

bound::Expr bindExpr(ast::FnCallExpr e, Context& ctx) {
    std::vector<bound::Expr> args;
    args.reserve(e.args.size());
    for (auto&& arg : e.args) {
        args.push_back(bindExpr(std::move(arg), ctx));
    }

    auto fields = requiredFieldsOf(args);
    auto level = ExprKindLevel::Const;
    for (auto&& arg : args) {
        require(
            composable(level, arg.level),
            "different expression kinds not allowed in function calls");
        level = composed(level, arg.level);
    }

    if (auto un_aggr_type = unaryAggregateType(e.func)) {
        require(args.size() == 1, "function expects 1 argument");
        require(
            args[0].level != ExprKindLevel::Group, "grouping operations do not accept aggregates");
        auto value_type = unaryAggregateValueType(*un_aggr_type, args[0].value_type);

        return {
            .node =
                bound::UnaryAggregateExpr{
                    .type = *un_aggr_type,
                    .expr = box(std::move(args[0])),
                },
            .value_type = value_type,
            .level = ExprKindLevel::Group,
            .required_fields = fields,
        };
    }

    if (e.func == "builtin_count_all") {
        require(args.empty(), "no arguments expected for COUNT(*)");

        return {
            .node = bound::CountAllExpr{},
            .value_type = ValueType::Integer,
            .level = ExprKindLevel::Group,
            .required_fields = FieldSet::emptySet(),
        };
    }

    if (e.func == "builtin_coalesce") {
        std::unordered_set<ValueType> types;
        for (auto&& arg : args) {
            if (arg.value_type != ValueType::Null) {
                types.insert(arg.value_type);
            }
        }
        require(args.size() >= 1, "at least one argument required for COALESCE");
        require(types.size() <= 1, "COALESCE arguments must have the same type");

        return {
            .node = bound::CoalesceExpr{.args = std::move(args)},
            .value_type = types.empty() ? ValueType::Null : *types.begin(),
            .level = level,
            .required_fields = fields,
        };
    }

    if (e.func == "builtin_percentile") {
        require(args.size() > 1, "PERCENTILE must be given at least one percentile");
        require(args[0].level != ExprKindLevel::Group, "PERCENTILE does not accept aggregates");
        require(
            dispatch<bool>(
                []<typename T>(std::type_identity<T>) {
                    return PercentileTraits::template allowed<T>();
                },
                args[0].value_type),
            "unsupported argument type for PERCENTILE");

        std::vector<float> percentiles;
        percentiles.reserve(args.size() - 1);
        for (size_t i = 1; i < args.size(); ++i) {
            require(
                args[i].value_type == ValueType::Floating,
                "PERCENTILE's arguments in positions >=1 must be floating");

            percentiles.push_back(
                util::match(
                    std::move(args[i].node),
                    [](bound::ValueExpr e) -> float {
                        auto p = e.value.get<float>();
                        require(p >= 0.0f && p <= 1.0f, "PERCENTILE value must be in [0, 1]");
                        return p;
                    },
                    [](auto) -> float {
                        throwError("PERCENTILE's arguments in positions >=1 must be literals");
                    }));
        }

        return {
            .node =
                bound::PercentileExpr{
                    .expr = box(std::move(args[0])),
                    .percentiles = std::move(percentiles),
                },
            .value_type = ValueType::String,
            .level = ExprKindLevel::Group,
            .required_fields = fields,
        };
    }

    if (e.func == "builtin_rsubstr") {
        require(args.size() == 2, "RSUBSTR expects exactly 2 arguments");
        require(
            args[0].value_type == ValueType::String, "RSUBSTR's first argument should be String");
        require(
            args[1].level == ExprKindLevel::Const, "RSUBSTR's second argument should be String");
        require(
            args[1].value_type == ValueType::String,
            "RSUBSTR's second argument should be floating");

        std::string regex = util::match(
            args[1].node,
            [](bound::ValueExpr e) { return e.value.get<std::string>(); },
            [](auto&&) -> std::string {
                throwError("RSUBSTR's second argument should be literal");
            });

        return {
            .node =
                bound::RSubstrExpr{
                    .expr = box(std::move(args[0])),
                    .regex = std::move(regex),
                },
            .value_type = ValueType::String,
            .level = level,
            .required_fields = fields,
        };
    }

    throwError("unknown function name: {}", e.func);
}

bound::Expr bindExpr(ast::BinaryExpr e, Context& ctx) {
    auto type = exprType(e.type);
    auto left = bindExpr(std::move(*e.left), ctx);
    auto right = bindExpr(std::move(*e.right), ctx);
    auto value_type = valueType(left.value_type, right.value_type, type);

    require(
        composable(left.level, right.level),
        "Binary operations require same expression level on both sides");

    auto level = composed(left.level, right.level);
    auto req_fields = FieldSet::merge(left.required_fields, right.required_fields);

    return {
        .node =
            bound::BinaryExpr{
                .type = type,
                .left = box(std::move(left)),
                .right = box(std::move(right)),
            },
        .value_type = value_type,
        .level = level,
        .required_fields = req_fields,
    };
}

bound::Expr bindExpr(ast::UnaryExpr e, Context& ctx) {
    auto arg = bindExpr(std::move(*e.expr), ctx);
    auto type = exprType(e.type);
    auto value_type = valueType(arg.value_type, type);
    auto level = arg.level;
    auto req_fields = arg.required_fields;

    return {
        .node =
            bound::UnaryExpr{
                .type = type,
                .expr = box(std::move(arg)),
            },
        .value_type = value_type,
        .level = level,
        .required_fields = req_fields,
    };
}

bound::Expr bindExpr(ast::Expr expr, Context& ctx) {
    return util::match(std::move(expr), [&](auto e) { return bindExpr(std::move(e), ctx); });
}

}  // namespace lsql::front::sql::bind

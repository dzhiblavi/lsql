#pragma once

#include "front/common/ast/Expressions.h"
#include "front/common/bind/Context.h"
#include "front/common/bound/ExprKindLevel.h"
#include "front/common/bound/Expressions.h"
#include "front/common/source/SourceSpan.h"
#include "front/common/source/require_at.h"

#include "core/exprs/BinaryExpr.h"
#include "core/exprs/Percentile.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/exprs/UnaryExpr.h"

namespace lsql::front::common::bind {

UnaryExprType exprType(ast::UnaryExprType ast);
BinaryExprType exprType(ast::BinaryExprType ast);
ValueType valueType(ValueType arg, UnaryExprType type);
ValueType valueType(ValueType l, ValueType r, BinaryExprType type);
std::optional<UnaryAggregateType> unaryAggregateType(std::string_view fn_name);
ValueType unaryAggregateValueType(UnaryAggregateType type, ValueType arg);

template <typename BoundExpr, typename AstExpr>
std::vector<BoundExpr> bindExprs(std::vector<AstExpr> exprs, auto& binder, Context& ctx) {
    std::vector<BoundExpr> result;
    result.reserve(exprs.size());
    for (auto&& p : exprs) {
        result.push_back(binder(std::move(p), ctx));
    }
    return result;
}

template <typename E>
concept BoundExpr = requires(const E& e) {
    { e.value_type } -> std::same_as<const ValueType&>;
    { e.level } -> std::same_as<const bound::ExprKindLevel&>;
    { e.required_fields } -> std::same_as<const FieldSet&>;
};

template <typename R>
concept BoundRel = requires(const R& r) {
    { r.fields_out } -> std::same_as<const bound::FieldSetNodePtr&>;
};

struct BoundExprInfo {
    ValueType value_type{};
    bound::ExprKindLevel level{};
    FieldSet required_fields{};
};

template <BoundExpr E>
FieldSet requiredFieldsOf(const std::vector<E>& exprs) {
    auto fields = FieldSet::emptySet();
    for (auto&& e : exprs) {
        fields.merge(e.required_fields);
    }
    return fields;
}

template <BoundExpr Arg, BoundRel Match>
std::pair<BoundExprInfo, FieldId> bindInExpr(
    const Arg& arg,
    const Match& match,
    auto& ctx,
    SourceSpan arg_span = {},
    SourceSpan match_span = {}) {
    requireAt(
        arg.level != common::bound::ExprKindLevel::Group,
        arg_span,
        "in key expression cannot be aggregate");
    auto req_fields = arg.required_fields;

    auto count = match.fields_out->fieldSet().fieldIds().size();
    requireAt(count == 1, match_span, "in match should contain one column, got {}", count);

    auto match_field_id = *match.fields_out->fieldSet().fieldIds().begin();
    requireAt(
        arg.value_type == ctx.binding()->type(match_field_id), arg_span, "in key type mismatch");

    return {
        BoundExprInfo{
            .value_type = ValueType::Boolean,
            .level = bound::ExprKindLevel::Row,
            .required_fields = req_fields,
        },
        match_field_id,
    };
}

template <BoundExpr Arg>
BoundExprInfo bindLikeExpr(const Arg& arg, SourceSpan arg_span = {}) {
    requireAt(arg.value_type == ValueType::String, arg_span, "like argument should be String");

    return {
        .value_type = ValueType::Boolean,
        .level = arg.level,
        .required_fields = arg.required_fields,
    };
}

template <BoundExpr Arg>
std::pair<BoundExprInfo, std::string> bindRsubstr(
    const std::vector<Arg>& args, SourceSpan args_span = {}) {
    requireAt(args.size() == 2, args_span, "rsubstr expects exactly 2 arguments");
    requireAt(
        args[0].value_type == ValueType::String,
        args_span,
        "rsubstr's first argument should be String");
    requireAt(
        args[1].level == common::bound::ExprKindLevel::Const,
        args_span,
        "rsubstr's second argument should be a constant");
    requireAt(
        args[1].value_type == ValueType::String,
        args_span,
        "rsubstr's second argument should be String");

    std::string regex = util::match(
        args[1].node,
        [](bound::ValueExpr e) { return e.value.get<std::string>(); },
        [&](auto&&) -> std::string {
            throwAt(args_span, "rsubstr's second argument should be literal");
        });

    return {
        BoundExprInfo{
            .value_type = ValueType::String,
            .level = args[0].level,
            .required_fields = args[0].required_fields,
        },
        std::move(regex),
    };
}

template <BoundExpr Arg>
std::pair<BoundExprInfo, UnaryExprType> bindUnaryExpr(const Arg& arg, ast::UnaryExprType type) {
    auto bound_type = common::bind::exprType(type);

    return {
        BoundExprInfo{
            .value_type = common::bind::valueType(arg.value_type, bound_type),
            .level = arg.level,
            .required_fields = arg.required_fields,
        },
        bound_type,
    };
}

template <BoundExpr L, BoundExpr R>
std::pair<BoundExprInfo, BinaryExprType> bindBinaryExpr(
    const L& l, const R& r, ast::BinaryExprType type, SourceSpan span = {}) {
    auto bound_type = common::bind::exprType(type);
    auto value_type = common::bind::valueType(l.value_type, r.value_type, bound_type);

    requireAt(
        composable(l.level, r.level),
        span,
        "binary operations require same expression level on both sides");

    auto level = composed(l.level, r.level);
    auto req_fields = FieldSet::merge(l.required_fields, r.required_fields);

    return {
        BoundExprInfo{
            .value_type = value_type,
            .level = level,
            .required_fields = req_fields,
        },
        bound_type,
    };
}

template <BoundExpr Arg>
BoundExprInfo bindUnaryAggregate(
    const std::vector<Arg>& args, UnaryAggregateType type, SourceSpan args_span = {}) {
    requireAt(args.size() == 1, args_span, "function expects 1 argument");
    requireAt(
        args[0].level != common::bound::ExprKindLevel::Group,
        args_span,
        "grouping operations do not accept aggregates");

    return {
        .value_type = common::bind::unaryAggregateValueType(type, args[0].value_type),
        .level = bound::ExprKindLevel::Group,
        .required_fields = args[0].required_fields,
    };
}

template <BoundExpr Arg>
BoundExprInfo bindCast(const Arg& arg, ValueType cast_to) {
    return {
        .value_type = cast_to,
        .level = arg.level,
        .required_fields = arg.required_fields,
    };
}

template <BoundExpr Arg>
BoundExprInfo bindCountAll(const std::vector<Arg>& args, SourceSpan args_span = {}) {
    requireAt(args.empty(), args_span, "no arguments expected for COUNT(*)");

    return {
        .value_type = ValueType::Integer,
        .level = bound::ExprKindLevel::Group,
        .required_fields = FieldSet::emptySet(),
    };
}

template <BoundExpr Arg>
BoundExprInfo bindCoalesce(const std::vector<Arg>& args, SourceSpan args_span = {}) {
    requireAt(args.size() >= 1, args_span, "at least one argument required for coalesce");

    auto level = common::bound::ExprKindLevel::Const;
    for (auto&& arg : args) {
        requireAt(
            composable(level, arg.level),
            args_span,
            "different expression kinds not allowed in function calls");
        level = composed(level, arg.level);
    }

    std::unordered_set<ValueType> types;
    for (auto&& arg : args) {
        if (arg.value_type != ValueType::Null) {
            types.insert(arg.value_type);
        }
    }

    requireAt(types.size() <= 1, args_span, "coalesce arguments must have the same type");

    return {
        .value_type = types.empty() ? ValueType::Null : *types.begin(),
        .level = level,
        .required_fields = requiredFieldsOf(args),
    };
}

template <BoundExpr Arg>
std::pair<BoundExprInfo, std::vector<float>> bindPercentile(
    const std::vector<Arg>& args, SourceSpan args_span = {}) {
    requireAt(args.size() > 1, args_span, "percentile must be given at least one percentile");
    requireAt(
        args[0].level != common::bound::ExprKindLevel::Group,
        args_span,
        "percentile does not accept aggregates");
    requireAt(
        dispatch<bool>(
            []<typename T>(std::type_identity<T>) {
                return PercentileTraits::template allowed<T>();
            },
            args[0].value_type),
        args_span,
        "unsupported argument type for percentile");

    std::vector<float> percentiles;
    percentiles.reserve(args.size() - 1);
    for (size_t i = 1; i < args.size(); ++i) {
        requireAt(
            args[i].value_type == ValueType::Floating,
            args_span,
            "percentile's arguments in positions >=1 must be floating");

        auto* value = std::get_if<bound::ValueExpr>(&args[i].node);
        if (!value) {
            throwAt(args_span, "percentile's arguments in positions >=1 must be literals");
        }

        auto p = value->value.template get<float>();
        requireAt(p >= 0.0f && p <= 1.0f, args_span, "percentile value must be in [0, 1]");
        percentiles.push_back(p);
    }

    return {
        BoundExprInfo{
            .value_type = ValueType::String,
            .level = bound::ExprKindLevel::Group,
            .required_fields = args[0].required_fields,
        },
        std::move(percentiles),
    };
}

}  // namespace lsql::front::common::bind

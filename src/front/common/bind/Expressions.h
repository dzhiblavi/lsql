#pragma once

#include "front/common/ast/Expressions.h"
#include "front/common/bind/Context.h"
#include "front/common/bound/BinaryExpr.h"
#include "front/common/bound/ExprKindLevel.h"
#include "front/common/bound/Expressions.h"
#include "front/common/bound/UnaryExpr.h"
#include "front/common/source/SourceSpan.h"
#include "front/common/source/require_at.h"

#include "core/function/Function.h"

#include <algorithm>

namespace lsql::front::common::bind {

bound::UnaryExprType exprType(ast::UnaryExprType ast);
bound::BinaryExprType exprType(ast::BinaryExprType ast);
ValueType valueType(ValueType arg, bound::UnaryExprType type, SourceSpan span);
ValueType valueType(ValueType l, ValueType r, bound::BinaryExprType type, SourceSpan span);

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

template <BoundExpr Arg>
Value getLiteral(const Arg& arg, SourceSpan span) {
    return util::match(
        arg.node,
        [](bound::ValueExpr e) -> Value { return e.value; },
        [&](auto&&) -> Value { throwAt(span, "expected literal argument"); });
}

template <BoundExpr Arg>
bound::ExprKindLevel composedLevel(std::span<const Arg> args, SourceSpan span) {
    auto level = common::bound::ExprKindLevel::Const;
    for (auto&& arg : args) {
        requireAt(composable(level, arg.level), span, "different expression kinds not allowed");
        level = composed(level, arg.level);
    }
    return level;
}

template <BoundExpr Arg>
std::tuple<BoundExprInfo, func::Function, std::vector<Arg>> bindFnCallExpr(
    std::string_view fn_name, std::vector<Arg> args, SourceSpan span, SourceSpan args_span) {
    auto level = composedLevel<Arg>(args, args_span);

    if (fn_name == "substr") {
        requireAt(2 <= args.size() && args.size() <= 3, args_span, "2-3 arguments required");
        requireAt(args[0].value_type == ValueType::String, args_span, "1st arg should be string");
        requireAt(args[1].value_type == ValueType::Integer, args_span, "2nd arg should be integer");
        if (args.size() == 3) {
            requireAt(
                args[2].value_type == ValueType::Integer, args_span, "3rd arg should be integer");
        }

        std::vector<Arg> dynamic;
        dynamic.push_back(std::move(args[0]));

        auto from = getLiteral(args[1], args_span).template get<int64_t>();
        requireAt(from >= 0, args_span, "negative pos is not allowed");

        auto length = std::string::npos;
        if (args.size() == 3) {
            length = getLiteral(args[2], args_span).template get<int64_t>();
            requireAt(length >= 0, args_span, "negative length is not allowed");
        }

        auto substr = func::Substr{
            .from = size_t(from),
            .length = length,
        };

        auto fields = requiredFieldsOf(dynamic);
        return {
            BoundExprInfo{
                .value_type = ValueType::String,
                .level = level,
                .required_fields = fields,
            },
            std::move(substr),
            std::move(dynamic),
        };
    }

    if (fn_name == "coalesce") {
        std::unordered_set<ValueType> types;
        for (auto&& arg : args) {
            if (arg.value_type != ValueType::Null) {
                types.insert(arg.value_type);
            }
        }
        requireAt(types.size() <= 1, args_span, "coalesce arguments must have the same type");

        return {
            BoundExprInfo{
                .value_type = types.empty() ? ValueType::Null : *types.begin(),
                .level = level,
                .required_fields = requiredFieldsOf(args),
            },
            func::Coalesce{},
            std::move(args),
        };
    }

    if (fn_name == "percentile") {
        requireAt(args.size() > 1, args_span, "percentile must be given at least one percentile");
        requireAt(
            args[0].level != common::bound::ExprKindLevel::Group,
            args_span,
            "percentile does not accept aggregates");
        requireAt(
            dispatch<bool>(
                []<typename T>(std::type_identity<T>) { return Comparable<T>; },
                args[0].value_type),
            args_span,
            "percentile argument must be comparable");

        std::vector<float> percentiles;
        percentiles.reserve(args.size() - 1);
        for (size_t i = 1; i < args.size(); ++i) {
            requireAt(
                args[i].value_type == ValueType::Floating,
                args_span,
                "percentile's arguments in positions >=1 must be floating");

            auto p = getLiteral(args[i], args_span).template get<float>();
            requireAt(p >= 0.0f && p <= 1.0f, args_span, "percentile value must be in [0, 1]");
            percentiles.push_back(p);
        }

        std::vector<Arg> dynamic;
        dynamic.push_back(std::move(args[0]));
        auto fields = requiredFieldsOf(dynamic);
        auto args_type = dynamic[0].value_type;

        return {
            BoundExprInfo{
                .value_type = ValueType::String,
                .level = bound::ExprKindLevel::Group,
                .required_fields = fields,
            },
            func::Percentile{
                .percentiles = std::move(percentiles),
                .args_type = args_type,
            },
            std::move(dynamic),
        };
    }

    if (fn_name == "rsubstr") {
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

        auto regex = std::string(getLiteral(args[1], args_span).template get<std::string_view>());
        std::vector<Arg> dynamic;
        dynamic.push_back(std::move(args[0]));
        auto fields = requiredFieldsOf(dynamic);

        return {
            BoundExprInfo{
                .value_type = ValueType::String,
                .level = level,
                .required_fields = fields,
            },
            func::RSubstr{.regex = std::move(regex)},
            std::move(dynamic),

        };
    }

    if (fn_name == "count_all") {
        requireAt(args.size() == 0, args_span, "no arguments expected for count_all");

        return {
            BoundExprInfo{
                .value_type = ValueType::Integer,
                .level = bound::ExprKindLevel::Group,
                .required_fields = FieldSet::emptySet(),
            },
            func::CountAll(),
            std::vector<Arg>(),
        };
    }

    if (fn_name == "count_nonnull") {
        requireAt(args.size() == 1, args_span, "1 argument expected for count_nonnull");
        auto fields = args[0].required_fields;

        return {
            BoundExprInfo{
                .value_type = ValueType::Integer,
                .level = bound::ExprKindLevel::Group,
                .required_fields = fields,
            },
            func::CountNonNull(),
            std::move(args),
        };
    }

    if (fn_name == "min") {
        requireAt(args.size() == 1, args_span, "function expects 1 argument");
        requireAt(
            args[0].level != common::bound::ExprKindLevel::Group,
            args_span,
            "min operation does not accept aggregates");

        auto type = args[0].value_type;
        auto fields = args[0].required_fields;

        return {
            BoundExprInfo{
                .value_type = type,
                .level = bound::ExprKindLevel::Group,
                .required_fields = fields,
            },
            func::Min{.arg_type = type},
            std::move(args),
        };
    }

    if (fn_name == "max") {
        requireAt(args.size() == 1, args_span, "function expects 1 argument");
        requireAt(
            args[0].level != common::bound::ExprKindLevel::Group,
            args_span,
            "max operation does not accept aggregates");

        auto type = args[0].value_type;
        auto fields = args[0].required_fields;

        return {
            BoundExprInfo{
                .value_type = type,
                .level = bound::ExprKindLevel::Group,
                .required_fields = fields,
            },
            func::Max{.arg_type = type},
            std::move(args),
        };
    }

    if (fn_name == "sum") {
        requireAt(args.size() == 1, args_span, "function expects 1 argument");
        requireAt(
            args[0].level != common::bound::ExprKindLevel::Group,
            args_span,
            "sum operation does not accept aggregates");

        auto type = args[0].value_type;
        auto fields = args[0].required_fields;

        return {
            BoundExprInfo{
                .value_type = type,
                .level = bound::ExprKindLevel::Group,
                .required_fields = fields,
            },
            func::Sum{.arg_type = type},
            std::move(args),
        };
    }

    if (fn_name == "parse_timestamp") {
        requireAt(args.size() == 2, args_span, "function expects 2 arguments");
        requireAt(args[0].value_type == ValueType::String, args_span, "the first argument must be string");
        requireAt(
            args[1].level == bound::ExprKindLevel::Const,
            args_span,
            "the second argument must be constant");
        requireAt(args[1].value_type == ValueType::String, args_span, "the argument must be string");

        auto str_format = getLiteral(args[1], args_span).template get<std::string_view>();
        auto maybe_format = magic_enum::enum_cast<TimeFormat>(str_format);
        requireAt(maybe_format.has_value(), args_span, "invalid time format '{}'", str_format);

        std::vector<Arg> dynamic;
        dynamic.push_back(std::move(args[0]));
        auto fields = requiredFieldsOf(dynamic);

        return {
            BoundExprInfo{
                .value_type = ValueType::Integer,
                .level = dynamic[0].level,
                .required_fields = fields,
            },
            func::ParseTimestamp{.format = *maybe_format},
            std::move(dynamic),
        };
    }

    static constexpr std::array<std::pair<std::string_view, ValueType>, 5> cast_fns{
        std::make_pair("null", ValueType::Null),
        std::make_pair("int", ValueType::Integer),
        std::make_pair("float", ValueType::Floating),
        std::make_pair("bool", ValueType::Boolean),
        std::make_pair("string", ValueType::String),
    };

    if (auto it = std::ranges::find(cast_fns, fn_name, [](auto&& p) { return p.first; });
        it != cast_fns.end()) {
        requireAt(args.size() == 1, args_span, "'{}' expects 1 argument", fn_name);
        auto fields = requiredFieldsOf(args);

        return {
            BoundExprInfo{
                .value_type = it->second,
                .level = level,
                .required_fields = fields,
            },
            func::Cast{.cast_to = it->second},
            std::move(args),
        };
    }

    throwAt(span, "unknown function name '{}'", fn_name);
}

template <BoundExpr Arg, BoundRel Match>
std::pair<BoundExprInfo, FieldId> bindInExpr(
    const Arg& arg, const Match& match, auto& ctx, SourceSpan arg_span, SourceSpan match_span) {
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
BoundExprInfo bindLikeExpr(const Arg& arg, SourceSpan arg_span) {
    requireAt(arg.value_type == ValueType::String, arg_span, "like argument should be String");

    return {
        .value_type = ValueType::Boolean,
        .level = arg.level,
        .required_fields = arg.required_fields,
    };
}

template <BoundExpr Arg>
std::pair<BoundExprInfo, bound::UnaryExprType> bindUnaryExpr(
    const Arg& arg, ast::UnaryExprType type, SourceSpan span) {
    auto bound_type = exprType(type);

    return {
        BoundExprInfo{
            .value_type = valueType(arg.value_type, bound_type, span),
            .level = arg.level,
            .required_fields = arg.required_fields,
        },
        bound_type,
    };
}

template <BoundExpr L, BoundExpr R>
std::pair<BoundExprInfo, bound::BinaryExprType> bindBinaryExpr(
    const L& l, const R& r, ast::BinaryExprType type, SourceSpan span) {
    auto bound_type = exprType(type);
    auto value_type = valueType(l.value_type, r.value_type, bound_type, span);

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
BoundExprInfo bindCast(const Arg& arg, ValueType cast_to) {
    return {
        .value_type = cast_to,
        .level = arg.level,
        .required_fields = arg.required_fields,
    };
}

template <BoundExpr Arg>
BoundExprInfo bindCountAll(const std::vector<Arg>& args, SourceSpan args_span) {
    requireAt(args.empty(), args_span, "no arguments expected for COUNT(*)");

    return {
        .value_type = ValueType::Integer,
        .level = bound::ExprKindLevel::Group,
        .required_fields = FieldSet::emptySet(),
    };
}

}  // namespace lsql::front::common::bind

#include "front/common/bind/Expressions.h"

#include "core/value/ValueType.h"
#include "util/enum.h"

#include <algorithm>
#include <magic_enum/magic_enum.hpp>

namespace lsql::front::common::bind {

UnaryExprType exprType(ast::UnaryExprType ast) {
    switch (ast) {
        using enum ast::UnaryExprType;

        case Not:
            return UnaryExprType::BooleanNegate;
    }
}

BinaryExprType exprType(ast::BinaryExprType ast) {
    switch (ast) {
        using enum ast::BinaryExprType;

        case Equal:
            return BinaryExprType::Equal;
        case NotEqual:
            return BinaryExprType::NotEqual;
        case And:
            return BinaryExprType::And;
        case Or:
            return BinaryExprType::Or;
        case Divide:
            return BinaryExprType::Divide;
        case Plus:
            return BinaryExprType::Add;
        case Minus:
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

template <UnaryExprType Type>
ValueType unaryExprResultType(ValueType value_type, SourceSpan span) {
    using Traits = UnaryExprTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename T>(std::type_identity<T>) {
            if constexpr (!Traits::template allowed<T>()) {
                throwAt(
                    span,
                    "unsupported operand type {} for unary operation {}",
                    magic_enum::enum_name(value_type),
                    magic_enum::enum_name(Type));
            }

            using ValueType = Traits::template ValueType<T>;
            return lsql::valueType<ValueType>();
        },
        value_type);
}

template <BinaryExprType Type>
ValueType binaryExprResultType(ValueType left, ValueType right, SourceSpan span) {
    using Traits = BinaryExprTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename L, typename R>(std::type_identity<L>, std::type_identity<R>) {
            if constexpr (!Traits::template allowed<L, R>()) {
                throwAt(
                    span,
                    "unsupported operand types {}, {} for binary operation {}",
                    magic_enum::enum_name(left),
                    magic_enum::enum_name(right),
                    magic_enum::enum_name(Type));
            }

            using ValueType = Traits::template ValueType<L, R>;
            return lsql::valueType<ValueType>();
        },
        left,
        right);
}

template <UnaryAggregateType Type>
ValueType unaryAggregateResultType(ValueType value_type, SourceSpan span) {
    using Traits = UnaryAggregateTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename T>(std::type_identity<T>) {
            if constexpr (!Traits::template allowed<T>()) {
                throwAt(
                    span,
                    "unsupported operand type {} for unary aggregate {}",
                    magic_enum::enum_name(value_type),
                    magic_enum::enum_name(Type));
            }

            using ValueType = Traits::template ValueType<T>;
            return lsql::valueType<ValueType>();
        },
        value_type);
}

ValueType valueType(ValueType arg, UnaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return unaryExprResultType<Type>(arg, span); }, type);
}

ValueType valueType(ValueType l, ValueType r, BinaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return binaryExprResultType<Type>(l, r, span); }, type);
}

ValueType unaryAggregateValueType(UnaryAggregateType type, ValueType arg, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return unaryAggregateResultType<Type>(arg, span); }, type);
}

}  // namespace lsql::front::common::bind

#include "front/common/bind/Expressions.h"

#include "core/ValueType.h"

#include "util/enum.h"
#include "util/require.h"

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
ValueType unaryExprResultType(ValueType value_type) {
    using Traits = UnaryExprTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename T>(std::type_identity<T>) {
            if constexpr (!Traits::template allowed<T>()) {
                throwError(
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
ValueType binaryExprResultType(ValueType left, ValueType right) {
    using Traits = BinaryExprTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename L, typename R>(std::type_identity<L>, std::type_identity<R>) {
            if constexpr (!Traits::template allowed<L, R>()) {
                throwError(
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
ValueType unaryAggregateResultType(ValueType value_type) {
    using Traits = UnaryAggregateTraits<Type>;

    return dispatch<ValueType>(
        [&]<typename T>(std::type_identity<T>) {
            if constexpr (!Traits::template allowed<T>()) {
                throwError(
                    "unsupported operand type {} for unary aggregate {}",
                    magic_enum::enum_name(value_type),
                    magic_enum::enum_name(Type));
            }

            using ValueType = Traits::template ValueType<T>;
            return lsql::valueType<ValueType>();
        },
        value_type);
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

}  // namespace lsql::front::common::bind

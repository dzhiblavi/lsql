#pragma once

#include "core/ValueType.h"
#include "core/exprs/BinaryExpr.h"
#include "core/exprs/UnaryAggregate.h"
#include "core/exprs/UnaryExpr.h"
#include "util/require.h"

#include <magic_enum/magic_enum.hpp>

namespace lsql::front {

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
            return valueType<ValueType>();
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
            return valueType<ValueType>();
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
            return valueType<ValueType>();
        },
        value_type);
}

}  // namespace lsql::front

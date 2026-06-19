#include "front/common/bind/Expressions.h"

#include "core/value/ValueType.h"
#include "util/enum.h"

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

ValueType valueType(ValueType arg, UnaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return unaryExprResultType<Type>(arg, span); }, type);
}

ValueType valueType(ValueType l, ValueType r, BinaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return binaryExprResultType<Type>(l, r, span); }, type);
}

}  // namespace lsql::front::common::bind

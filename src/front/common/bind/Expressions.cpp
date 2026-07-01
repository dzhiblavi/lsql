#include "front/common/bind/Expressions.h"

#include "core/value/ValueType.h"
#include "util/enum.h"

#include <magic_enum/magic_enum.hpp>

namespace lsql::front::common::bind {

bound::UnaryExprType exprType(ast::UnaryExprType ast) {
    switch (ast) {
        using enum ast::UnaryExprType;

        case Not:
            return bound::UnaryExprType::BooleanNegate;
    }
}

bound::BinaryExprType exprType(ast::BinaryExprType ast) {
    switch (ast) {
        using enum ast::BinaryExprType;

        case Equal:
            return bound::BinaryExprType::Equal;
        case NotEqual:
            return bound::BinaryExprType::NotEqual;
        case Less:
            return bound::BinaryExprType::Less;
        case Greater:
            return bound::BinaryExprType::Greater;
        case LessEqual:
            return bound::BinaryExprType::LessEqual;
        case GreaterEqual:
            return bound::BinaryExprType::GreaterEqual;
        case And:
            return bound::BinaryExprType::And;
        case Or:
            return bound::BinaryExprType::Or;
        case Divide:
            return bound::BinaryExprType::Divide;
        case Plus:
            return bound::BinaryExprType::Add;
        case Minus:
            return bound::BinaryExprType::Subtract;
    }
}

template <bound::UnaryExprType Type>
ValueType unaryExprResultType(ValueType value_type, SourceSpan span) {
    using Traits = bound::UnaryExprTraits<Type>;

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

template <bound::BinaryExprType Type>
ValueType binaryExprResultType(ValueType left, ValueType right, SourceSpan span) {
    using Traits = bound::BinaryExprTraits<Type>;

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

ValueType valueType(ValueType arg, bound::UnaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return unaryExprResultType<Type>(arg, span); }, type);
}

ValueType valueType(ValueType l, ValueType r, bound::BinaryExprType type, SourceSpan span) {
    return util::enum_dispatch(
        [&]<auto Type>() { return binaryExprResultType<Type>(l, r, span); }, type);
}

}  // namespace lsql::front::common::bind

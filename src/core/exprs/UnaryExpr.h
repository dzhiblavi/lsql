#pragma once

#include <concepts>

namespace lsql {

enum class UnaryExprType {
    BooleanNegate,
};

template <UnaryExprType Type>
struct UnaryExprTraits;

template <>
struct UnaryExprTraits<UnaryExprType::BooleanNegate> {
    template <typename T>
    static constexpr bool allowed() {
        return std::same_as<bool, T>;
    }

    template <typename T>
    using ValueType = bool;
};

}  // namespace lsql

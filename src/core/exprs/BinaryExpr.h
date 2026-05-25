#pragma once

#include "core/exprs/concepts.h"
#include "core/null_t.h"

namespace lsql {

enum class BinaryExprType {
    Equal,
    NotEqual,
    And,
    Or,
    Divide,
    Add,
    Subtract,
};

template <BinaryExprType Type>
struct BinaryExprTraits;

template <>
struct BinaryExprTraits<BinaryExprType::Equal> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        if (std::same_as<L, null_t> || std::same_as<R, null_t>) {
            return true;
        }

        return std::same_as<L, R>;
    }

    template <typename L, typename R>
    using ValueType = bool;
};

template <>
struct BinaryExprTraits<BinaryExprType::NotEqual> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        if (std::same_as<L, null_t> || std::same_as<R, null_t>) {
            return true;
        }

        return std::same_as<L, R>;
    }

    template <typename L, typename R>
    using ValueType = bool;
};

template <>
struct BinaryExprTraits<BinaryExprType::And> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        return std::same_as<L, bool> && std::same_as<R, bool>;
    }

    template <typename L, typename R>
    using ValueType = bool;
};

template <>
struct BinaryExprTraits<BinaryExprType::Or> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        return std::same_as<L, bool> && std::same_as<R, bool>;
    }

    template <typename L, typename R>
    using ValueType = bool;
};

template <>
struct BinaryExprTraits<BinaryExprType::Divide> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        return std::same_as<L, R> && Dividable<L>;
    }

    template <typename L, typename R>
    using ValueType = L;
};

template <>
struct BinaryExprTraits<BinaryExprType::Add> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        return std::same_as<L, R> && Addable<L>;
    }

    template <typename L, typename R>
    using ValueType = L;
};

template <>
struct BinaryExprTraits<BinaryExprType::Subtract> {
    template <typename L, typename R>
    static constexpr bool allowed() {
        return std::same_as<L, R> && Subtractable<L>;
    }

    template <typename L, typename R>
    using ValueType = L;
};

}  // namespace lsql

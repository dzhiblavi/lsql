#pragma once

#include "core/exprs/concepts.h"

namespace lsql {

enum class UnaryAggregateType {
    Count,
    Min,
    Max,
    Sum,
};

template <UnaryAggregateType Type>
struct UnaryAggregateTraits;

template <>
struct UnaryAggregateTraits<UnaryAggregateType::Count> {
    template <typename T>
    static constexpr bool allowed() {
        return std::same_as<bool, T>;
    }

    template <typename T>
    using ValueType = int64_t;
};

template <>
struct UnaryAggregateTraits<UnaryAggregateType::Min> {
    template <typename T>
    static constexpr bool allowed() {
        return Comparable<T>;
    }

    template <typename T>
    using ValueType = T;
};

template <>
struct UnaryAggregateTraits<UnaryAggregateType::Max> {
    template <typename T>
    static constexpr bool allowed() {
        return Comparable<T>;
    }

    template <typename T>
    using ValueType = T;
};

template <>
struct UnaryAggregateTraits<UnaryAggregateType::Sum> {
    template <typename T>
    static constexpr bool allowed() {
        return Addable<T>;
    }

    template <typename T>
    using ValueType = T;
};

}  // namespace lsql

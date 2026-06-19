#pragma once

#include "core/value/Value.h"

#include <span>

namespace lsql::func {

template <typename A>
concept Aggregator = requires(A& a, std::span<Value> args) {
    { std::move(a).get() } -> std::same_as<Value>;
    { a.feed(args) } -> std::same_as<void>;
};

template <typename A>
concept Aggregate = requires(A& a) {
    typename A::Aggregator;
    { a.aggregator() } -> std::same_as<typename A::Aggregator>;
};

}  // namespace lsql::func

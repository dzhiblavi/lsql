#pragma once

#include "core/value/Value.h"

namespace lsql::func {

template <typename A>
concept NullaryAggregator = requires(A& a) {
    { std::move(a).get() } -> std::same_as<Value>;
    { a.feed() } -> std::same_as<void>;
};

template <typename A>
concept UnaryAggregator = requires(A& a, Value arg) {
    { std::move(a).get() } -> std::same_as<Value>;
    { a.feed(arg) } -> std::same_as<void>;
};

template <typename A>
concept NullaryAggregate = requires(A& a) {
    typename A::Aggregator;
    requires NullaryAggregator<typename A::Aggregator>;
    { a.aggregator() } -> std::same_as<typename A::Aggregator>;
};

template <typename A>
concept UnaryAggregate = requires(A& a) {
    typename A::Aggregator;
    requires UnaryAggregator<typename A::Aggregator>;
    { a.aggregator() } -> std::same_as<typename A::Aggregator>;
};

}  // namespace lsql::func

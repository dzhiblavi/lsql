#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Concepts.h"
#include "core/function/Function.h"

namespace lsql::func {

template <typename T>
struct MinAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(const Value& value) {
        if (value == vnull) {
            return;
        }

        auto&& next = value.get<T>();
        if (res.has_value()) {
            res = std::min(*res, U(next));
        } else {
            res = U(next);
        }
    }

    Value get() {
        if (res.has_value()) {
            return std::move(*res);
        }
        return vnull;
    }

    std::optional<U> res = std::nullopt;
};

static_assert(UnaryAggregator<MinAggregator<int64_t>>);

template <Comparable T>
struct MinAggregate {
    using Aggregator = MinAggregator<T>;
    MinAggregator<T> aggregator() const { return {}; }
};

static_assert(UnaryAggregate<MinAggregate<int64_t>>);

template <Comparable T>
MinAggregate<T> build(const Min& /*s*/) {
    return {};
}

}  // namespace lsql::func

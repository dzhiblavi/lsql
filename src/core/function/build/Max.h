#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Concepts.h"
#include "core/function/Function.h"

namespace lsql::func {

template <typename T>
struct MaxAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    void feed(const Value& value) {
        if (value == vnull) {
            return;
        }

        auto&& next = value.get<T>();
        if (res.has_value()) {
            res = std::max(*res, U(next));
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

static_assert(UnaryAggregator<MaxAggregator<int64_t>>);

template <Comparable T>
struct MaxAggregate {
    using Aggregator = MaxAggregator<T>;
    MaxAggregator<T> aggregator() const { return {}; }
};

static_assert(UnaryAggregate<MaxAggregate<int64_t>>);

template <Comparable T>
MaxAggregate<T> build(const Max& /*s*/) {
    return {};
}

}  // namespace lsql::func

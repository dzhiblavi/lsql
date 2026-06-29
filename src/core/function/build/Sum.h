#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Concepts.h"
#include "core/function/Function.h"

namespace lsql::func {

template <typename T>
struct SumAggregator {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::stringstream, T>;

    SumAggregator() = default;
    SumAggregator(SumAggregator<T>&&) noexcept = default;

    void feed(const Value& value) {
        if (value == vnull) {
            return;
        }

        if constexpr (std::same_as<U, T>) {
            sum += value.get<T>();
        } else {
            sum << value.get<std::string_view>();
        }
    }

    Value get() {
        if constexpr (std::same_as<U, T>) {
            return sum;
        } else {
            return sum.str();
        }
    }

    U sum = U();
};

static_assert(UnaryAggregator<SumAggregator<int64_t>>);

template <Addable T>
struct SumAggregate {
    using Aggregator = SumAggregator<T>;
    SumAggregator<T> aggregator() const { return {}; }
};

static_assert(UnaryAggregate<SumAggregate<int64_t>>);

template <Addable T>
SumAggregate<T> build(const Sum& /*s*/) {
    return {};
}

}  // namespace lsql::func

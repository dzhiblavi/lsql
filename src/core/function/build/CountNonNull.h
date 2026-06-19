#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Function.h"

namespace lsql::func {

struct CountNonNullAggregator {
    void feed(const Value& value) { count += value != vnull; }

    Value get() { return count; }

    int64_t count = 0;
};

static_assert(UnaryAggregator<CountNonNullAggregator>);

struct CountNonNullAggregate {
    using Aggregator = CountNonNullAggregator;
    CountNonNullAggregator aggregator() const { return {}; }
};

static_assert(UnaryAggregate<CountNonNullAggregate>);

inline CountNonNullAggregate build(const CountNonNull& /*s*/) {
    return {};
}

}  // namespace lsql::func

#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Function.h"

namespace lsql::func {

struct CountAllAggregator {
    void feed() { ++count; }
    Value get() { return count; }
    int64_t count = 0;
};

static_assert(NullaryAggregator<CountAllAggregator>);

struct CountAllAggregate {
    using Aggregator = CountAllAggregator;
    CountAllAggregator aggregator() const { return {}; }
};

static_assert(NullaryAggregate<CountAllAggregate>);

inline CountAllAggregate build(const CountAll& /*s*/) {
    return {};
}

}  // namespace lsql::func

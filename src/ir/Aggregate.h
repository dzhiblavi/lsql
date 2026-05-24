#pragma once

#include <variant>

namespace lsql::ir {

struct ScalarAggregate;
struct PercentileAggregate;

using AggregateNode = std::variant< //
    ScalarAggregate, //
    PercentileAggregate //
>;

struct Aggregate;

}  // namespace lsql::ir

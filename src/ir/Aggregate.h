#pragma once

#include <variant>

namespace lsql::ir {

struct UnaryAggregate;
struct PercentileAggregate;

using AggregateNode = std::variant< //
    UnaryAggregate, //
    PercentileAggregate //
>;

struct Aggregate;

}  // namespace lsql::ir

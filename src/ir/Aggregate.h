#pragma once

#include <variant>

namespace lsql::ir {

struct UnaryAggregate;
struct PercentileAggregate;
struct ConstAggregate;

using AggregateNode = std::variant< //
    UnaryAggregate, //
    PercentileAggregate, //
    ConstAggregate //
>;

struct Aggregate;

}  // namespace lsql::ir

#pragma once

#include <variant>

namespace lsql::ir {

struct UnaryAggregate;
struct CountAllAggregate;
struct PercentileAggregate;
struct ConstAggregate;

using AggregateNode = std::variant< //
    UnaryAggregate, //
    CountAllAggregate, //
    PercentileAggregate, //
    ConstAggregate //
>;

struct Aggregate;

}  // namespace lsql::ir

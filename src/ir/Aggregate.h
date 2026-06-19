#pragma once

#include <variant>

namespace lsql::ir {

struct FnCallAggregate;
struct ConstAggregate;

using AggregateNode = std::variant< //
    FnCallAggregate, //
    ConstAggregate //
>;

struct Aggregate;

}  // namespace lsql::ir

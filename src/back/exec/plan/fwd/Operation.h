#pragma once

#include <variant>

namespace lsql::back::exec::plan {

struct Aggregate;
struct Projection;
struct Filter;
struct Group;
struct Limit;
struct Log;
struct Stream;
struct MarkJoin;
struct Materialize;
struct MergeSorted;
struct SemiJoin;
struct Sort;
struct TopK;
struct UnionAll;
struct Values;

using OperationNode = std::variant< //
    Aggregate,
    Projection,
    Filter,
    Group,
    Limit,
    Log,
    Stream,
    MarkJoin,
    Materialize,
    MergeSorted,
    SemiJoin,
    Sort,
    TopK,
    UnionAll,
    Values
>;

struct Operation;

}  // namespace lsql::back::exec::plan

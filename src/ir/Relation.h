#pragma once

#include <variant>

namespace lsql::ir {

struct ValuesRelation;
struct ProjectionRelation;
struct AggregateRelation;
struct GroupRelation;
struct LimitRelation;
struct FilterRelation;
struct SortRelation;
struct SemiJoinRelation;
struct UnionAllRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct FileIntervalRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using Relation = std::variant< //
    ValuesRelation, //
    ProjectionRelation, //
    AggregateRelation, //
    GroupRelation, //
    LimitRelation, //
    FilterRelation, //
    SortRelation, //
    SemiJoinRelation, //
    UnionAllRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

}  // namespace lsql::ir

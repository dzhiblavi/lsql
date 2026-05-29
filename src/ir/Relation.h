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
struct TopKRelation;
struct SemiJoinRelation;
struct MarkJoinRelation;
struct UnionAllRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct FileIntervalRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    ValuesRelation, //
    ProjectionRelation, //
    AggregateRelation, //
    GroupRelation, //
    LimitRelation, //
    FilterRelation, //
    SortRelation, //
    TopKRelation, //
    SemiJoinRelation, //
    MarkJoinRelation, //
    UnionAllRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::ir

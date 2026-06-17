#pragma once

#include <variant>

namespace lsql::ir {

struct EmptyRelation;
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
struct StreamRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    EmptyRelation, //
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
    StreamRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::ir

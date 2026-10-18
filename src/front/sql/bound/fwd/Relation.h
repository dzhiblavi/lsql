#pragma once

#include <variant>

namespace lsql::front::sql::bound {

struct AdhocRelation;
struct SelectRelation;
struct AliasRelation;
struct UnionAllRelation;
struct LeftJoinRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct StreamRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    AdhocRelation, //
    SelectRelation, //
    AliasRelation,
    UnionAllRelation, //
    LeftJoinRelation,
    UnionAllSortedByRelation, //
    FileRelation, //
    StreamRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::front::sql::bound

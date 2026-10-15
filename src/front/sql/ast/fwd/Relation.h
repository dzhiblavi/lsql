#pragma once

#include <variant>

namespace lsql::front::sql::ast {

struct AdhocRelation;
struct SelectRelation;
struct AliasRelation;
struct UnionAllRelation;
struct LeftJoinRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct FileIntervalRelation;
struct StreamRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    AdhocRelation, //
    SelectRelation, //
    AliasRelation, //
    UnionAllRelation, //
    LeftJoinRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    StreamRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::front::sql::ast

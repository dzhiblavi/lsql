#pragma once

#include <variant>

namespace lsql::front::sql::bound {

struct AdhocRelation;
struct SelectRelation;
struct UnionAllRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct FileIntervalRelation;
struct StreamRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    AdhocRelation, //
    SelectRelation, //
    UnionAllRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    StreamRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::front::sql::bound

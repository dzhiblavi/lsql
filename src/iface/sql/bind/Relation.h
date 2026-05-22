#pragma once

#include <variant>

namespace lsql::iface::sql::bind {

struct AdhocRelation;
struct SelectRelation;
struct UnionAllRelation;
struct UnionAllSortedByRelation;
struct FileRelation;
struct FileIntervalRelation;
struct NamedRelationReferenceRelation;
struct MaterializeRelation;

using RelationNode = std::variant< //
    AdhocRelation, //
    SelectRelation, //
    UnionAllRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

struct Relation;

}  // namespace lsql::iface::sql::ast

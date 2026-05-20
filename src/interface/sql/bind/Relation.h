#pragma once

#include "interface/sql/ast/Relation.h"

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

using Relation = std::variant< //
    AdhocRelation, //
    SelectRelation, //
    UnionAllRelation, //
    UnionAllSortedByRelation, //
    FileRelation, //
    FileIntervalRelation, //
    NamedRelationReferenceRelation, //
    MaterializeRelation //
>;

}  // namespace lsql::iface::sql::bind

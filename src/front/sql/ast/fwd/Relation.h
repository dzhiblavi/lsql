#pragma once

#include <variant>

namespace lsql::front::sql::ast {

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

}  // namespace lsql::front::sql::ast

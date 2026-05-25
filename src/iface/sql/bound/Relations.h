#pragma once

#include "iface/sql/bound/FieldSetNode.h"
#include "iface/sql/bound/fwd/Expr.h"
#include "iface/sql/bound/fwd/Relation.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <optional>
#include <vector>

namespace lsql::iface::sql::bound {

struct StarProjector;
struct IdentifierProjector;
struct ExprProjector;

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct StarProjector {};

struct IdentifierProjector {
    FieldId field_id;
};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

struct Limit {
    int limit;
};

struct Where {
    Box<Expr> condition;
};

struct OrderBy {
    std::vector<Expr> order_list;
    bool desc;
};

struct GroupBy {
    std::vector<Projector> group_list;
};

struct AdhocRelation {
    std::vector<Value> values;
    FieldId output_field_id;
};

struct SelectRelation {
    std::vector<Projector> projectors;

    Box<Relation> source;
    std::optional<Limit> limit;
    std::optional<Where> where;
    std::optional<OrderBy> order_by;
    std::optional<GroupBy> group_by;
    bool aggregate;
};

struct UnionAllRelation {
    Box<Relation> left;
    Box<Relation> right;
};

struct UnionAllSortedByRelation {
    Box<Relation> left;
    Box<Relation> right;
    OrderBy order_by;
};

struct FileRelation {
    std::string path;
};

struct FileIntervalRelation {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

struct Relation {
    RelationNode node;
    FieldSetNodePtr fields_out;
};

}  // namespace lsql::iface::sql::bound

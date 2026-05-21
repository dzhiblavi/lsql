#pragma once

#include "ir/Expr.h"
#include "ir/Relation.h"
#include "ir/RelationFields.h"

#include "util/overloaded.h"

#include "core/Value.h"
#include "core/types.h"

#include <vector>

namespace lsql::ir {

RelationFields fieldsOf(const Relation& r);

struct AdhocRelation {
    std::vector<Value> values;
    RelationFields fields;
};

struct StarProjector {};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

using Projector = std::variant<StarProjector, ExprProjector>;

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

struct SelectRelation {
    std::vector<Projector> projectors;
    Box<Relation> source;
    RelationFields fields;
    bool aggregate;

    std::optional<Limit> limit;
    std::optional<Where> where;
    std::optional<OrderBy> order_by;
    std::optional<GroupBy> group_by;
};

struct UnionAllRelation {
    Box<Relation> left;
    Box<Relation> right;
    RelationFields fields;
};

struct UnionAllSortedByRelation {
    Box<Relation> left;
    Box<Relation> right;
    OrderBy order_by;
    RelationFields fields;
};

struct FileRelation {
    std::string path;
    RelationFields fields;
};

struct FileIntervalRelation {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
    RelationFields fields;
};

struct NamedRelationReferenceRelation {
    std::string name;
    RelationFields fields;
};

struct MaterializeRelation {
    Box<Relation> relation;
    RelationFields fields;
};

inline RelationFields fieldsOf(const Relation& r) {
    return util::match(r, [](auto&& r) { return r.fields; });
}

}  // namespace lsql::iface::sql::bind

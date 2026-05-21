#pragma once

#include "ir/Expr.h"
#include "ir/Relation.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <vector>

namespace lsql::ir {

struct StarProjector {};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

using Projector = std::variant<StarProjector, ExprProjector>;

struct ValuesRelation {
    std::vector<Value> values;
};

struct ProjectionRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
};

struct AggregateRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
};

struct GroupRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
    std::vector<Projector> group_list;
};

struct LimitRelation {
    Box<Relation> source;
    int limit;
};

struct FilterRelation {
    Box<Relation> source;
    Box<Expr> condition;
};

struct SortRelation {
    Box<Relation> source;
    std::vector<Expr> order_list;
    bool desc;
};

struct UnionAllRelation {
    Box<Relation> left;
    Box<Relation> right;
};

struct UnionAllSortedByRelation {
    Box<Relation> left;
    Box<Relation> right;
    std::vector<Expr> order_list;
    bool desc;
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

}  // namespace lsql::ir

#pragma once

#include "ir/Expr.h"
#include "ir/Relation.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <vector>

namespace lsql::ir {

struct Projector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

struct ValuesRelation {
    std::vector<Value> values;
    FieldId output_id;
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

struct SemiJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Expr> expr;
    FieldId match_field_id;
};

struct MarkJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Expr> expr;
    FieldId output_field_id;
    FieldId match_field_id;
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
    FieldSet requested_fields;
};

struct FileIntervalRelation {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
    FieldSet requested_fields;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

}  // namespace lsql::ir

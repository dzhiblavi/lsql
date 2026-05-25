#pragma once

#include "ir/Aggregate.h"
#include "ir/Relation.h"
#include "ir/Scalar.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <vector>

namespace lsql::ir {

struct Projector {
    FieldId alias_field_id;
    Box<Scalar> expr;
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
    std::vector<Aggregate> aggregates;
};

struct GroupRelation {
    Box<Relation> source;
    std::vector<Aggregate> aggregates;
    std::vector<Projector> group_list;
};

struct LimitRelation {
    Box<Relation> source;
    int limit;
};

struct FilterRelation {
    Box<Relation> source;
    Box<Scalar> condition;
};

struct SortRelation {
    Box<Relation> source;
    std::vector<Scalar> order_list;
    bool desc;
};

struct SemiJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Scalar> expr;
    FieldId match_field_id;
};

struct MarkJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Scalar> expr;
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
    std::vector<Scalar> order_list;
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

struct Relation {
    RelationNode node;
    FieldSet fields_out;
};

}  // namespace lsql::ir

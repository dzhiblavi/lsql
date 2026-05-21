#pragma once

#include "ir/Expr.h"
#include "ir/Relation.h"
#include "ir/RelationFields.h"

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
    FieldId output_id;
    RelationFields fields_out;
};

struct ProjectionRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
    RelationFields fields_out;
};

struct AggregateRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
    RelationFields fields_out;
};

struct GroupRelation {
    Box<Relation> source;
    std::vector<Projector> projectors;
    std::vector<Projector> group_list;
    RelationFields fields_out;
};

struct LimitRelation {
    Box<Relation> source;
    int limit;
    RelationFields fields_out;
};

struct FilterRelation {
    Box<Relation> source;
    Box<Expr> condition;
    RelationFields fields_out;
};

struct SortRelation {
    Box<Relation> source;
    std::vector<Expr> order_list;
    bool desc;
    RelationFields fields_out;
};

struct SemiJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Expr> expr;
    RelationFields fields_out;
};

struct MarkJoinRelation {
    Box<Relation> source;
    Box<Relation> match;
    Box<Expr> expr;
    FieldId output_field_id;
    RelationFields fields_out;
};

struct UnionAllRelation {
    Box<Relation> left;
    Box<Relation> right;
    RelationFields fields_out;
};

struct UnionAllSortedByRelation {
    Box<Relation> left;
    Box<Relation> right;
    std::vector<Expr> order_list;
    bool desc;
    RelationFields fields_out;
};

struct FileRelation {
    std::string path;
    RelationFields fields_out;
};

struct FileIntervalRelation {
    std::string path;
    timestamp_t ts_from;
    timestamp_t ts_to;
    RelationFields fields_out;
};

struct NamedRelationReferenceRelation {
    std::string name;
    RelationFields fields_out;
};

struct MaterializeRelation {
    Box<Relation> relation;
    RelationFields fields_out;
};

inline const RelationFields& fieldsOutOf(const Relation& r) {
    return util::match(r, [](const auto& r) -> const RelationFields& { return r.fields_out; });
}

inline RelationFields& fieldsOutOf(Relation& r) {
    return util::match(r, [](auto& r) -> RelationFields& { return r.fields_out; });
}

}  // namespace lsql::ir

#pragma once

#include "front/common/bound/RelationFieldSet.h"
#include "front/common/source/SourceSpan.h"

#include "front/sql/bound/fwd/Expr.h"
#include "front/sql/bound/fwd/Relation.h"

#include "core/types.h"
#include "core/value/Value.h"

#include <optional>
#include <vector>

namespace lsql::front::sql::bound {

struct StarProjector {};

struct IdentifierProjector {
    FieldId field_id;
};

struct ExprProjector {
    FieldId alias_field_id;
    Box<Expr> expr;
};

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

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
    std::optional<Where> having;
    std::optional<OrderBy> order_by;
    std::optional<GroupBy> group_by;
    bool aggregate;
};

struct AliasRelation {
    std::string alias;
    Box<Relation> source;
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

struct StreamRelation {
    std::string command;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

using common::bound::RelationFieldSetPtr;

struct Relation {
    RelationNode node;
    RelationFieldSetPtr fields_out;
    SourceSpan span;
};

}  // namespace lsql::front::sql::bound

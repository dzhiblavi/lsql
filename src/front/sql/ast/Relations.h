#pragma once

#include "front/sql/ast/fwd/Expr.h"
#include "front/sql/ast/fwd/Relation.h"

#include "front/Literal.h"

#include "core/types.h"

#include <optional>
#include <vector>

namespace lsql::front::sql::ast {

struct AdhocRelation {
    std::vector<Literal> literals;
};

struct StarProjector;
struct IdentifierProjector;
struct ExprProjector;

using Projector = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct StarProjector {};

struct IdentifierProjector {
    std::string identifier;
};

struct ExprProjector {
    std::string alias;
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

struct SelectRelation {
    std::vector<Projector> projectors;

    Box<Relation> source;
    std::optional<Limit> limit;
    std::optional<Where> where;
    std::optional<OrderBy> order_by;
    std::optional<GroupBy> group_by;
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
    std::string ts_from;
    int interval_s;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

}  // namespace lsql::front::sql::ast

#pragma once

#include "front/sql/ast/fwd/Expr.h"
#include "front/sql/ast/fwd/Relation.h"

#include "front/common/ast/Literal.h"

#include "front/common/source/SourceSpan.h"

#include "core/types.h"

#include <optional>
#include <vector>

namespace lsql::front::sql::ast {

struct AdhocRelation {
    std::vector<common::ast::Literal> literals;
};

struct StarProjector {};

struct IdentifierProjector {
    std::string identifier;
};

struct ExprProjector {
    std::string alias;
    Box<Expr> expr;
};

struct StarProjector;
struct IdentifierProjector;
struct ExprProjector;

using ProjectorNode = std::variant<StarProjector, IdentifierProjector, ExprProjector>;

struct Projector {
    ProjectorNode node;
    SourceSpan span;
};

struct Limit {
    int limit;
    SourceSpan span;
};

struct Where {
    Box<Expr> condition;
    SourceSpan span;
};

struct OrderBy {
    std::vector<Expr> order_list;
    bool desc;
    SourceSpan span;
};

struct GroupBy {
    std::vector<Projector> group_list;
    SourceSpan span;
};

struct SelectRelation {
    std::vector<Projector> projectors;

    Box<Relation> source;
    std::optional<Limit> limit;
    std::optional<Where> where;
    std::optional<Where> having;
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

struct StreamRelation {
    std::string command;
};

struct NamedRelationReferenceRelation {
    std::string name;
};

struct MaterializeRelation {
    Box<Relation> relation;
};

struct Relation {
    RelationNode node;
    SourceSpan span;
};

}  // namespace lsql::front::sql::ast

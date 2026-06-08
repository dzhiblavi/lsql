#pragma once

#include "front/sql/ast/fwd/Relation.h"

#include "front/common/source/SourceSpan.h"

#include "core/types.h"

#include <variant>
#include <vector>

namespace lsql::front::sql::ast {

struct NamedRelationStatement;
struct QueryStatement;

using StatementNode = std::variant<  //
    NamedRelationStatement,
    QueryStatement //
>;

struct NamedRelationStatement {
    std::string name;
    Box<Relation> relation;
};

struct QueryStatement {
    Box<Relation> relation;
};

struct Statement {
    StatementNode node;
    SourceSpan span;
};

using Program = std::vector<Statement>;

}  // namespace lsql::front::sql::ast

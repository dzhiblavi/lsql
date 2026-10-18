#pragma once

#include "front/sql/ast/fwd/Expr.h"
#include "front/sql/ast/fwd/Relation.h"

#include "front/common/source/SourceSpan.h"

#include "core/types.h"

#include <string>
#include <variant>
#include <vector>

namespace lsql::front::sql::ast {

struct NamedRelationStatement;
struct QueryStatement;
struct MacroDefinitionStatement;

using StatementNode = std::variant<  //
    NamedRelationStatement, //
    MacroDefinitionStatement, //
    QueryStatement //
>;

struct NamedRelationStatement {
    std::string name;
    Box<Relation> relation;
};

struct QueryStatement {
    Box<Relation> relation;
};

struct MacroParameter {
    std::string name;
    SourceSpan span;
};

struct MacroDefinitionStatement {
    std::string name;
    std::vector<MacroParameter> parameters;
    Box<Expr> body;
};

struct Statement {
    StatementNode node;
    SourceSpan span;
};

using Program = std::vector<Statement>;

}  // namespace lsql::front::sql::ast

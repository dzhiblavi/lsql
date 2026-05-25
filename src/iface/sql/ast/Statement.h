#pragma once

#include "iface/sql/ast/fwd/Relation.h"

#include "core/types.h"

#include <variant>
#include <vector>

namespace lsql::iface::sql::ast {

struct NamedRelationStatement;
struct QueryStatement;

using Statement = std::variant<  //
    NamedRelationStatement,
    QueryStatement //
>;

using Program = std::vector<Statement>;

struct NamedRelationStatement {
    std::string name;
    Box<Relation> relation;
};

struct QueryStatement {
    Box<Relation> relation;
};

}  // namespace lsql::iface::sql::ast

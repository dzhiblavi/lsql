#pragma once

#include "front/sql/bound/Relations.h"

#include "core/Fields.h"
#include "core/types.h"

#include <variant>
#include <vector>

namespace lsql::front::sql::bound {

struct NamedRelationStatement;
struct QueryStatement;

using Statement = std::variant<  //
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

struct Program {
    std::vector<Statement> statements;
    FieldBindingPtr binding;
};

}  // namespace lsql::front::sql::bound

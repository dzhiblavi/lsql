#pragma once

#include "ir/Relation.h"

#include "core/schema/Fields.h"
#include "core/types.h"

#include <variant>
#include <vector>

namespace lsql::ir {

struct NamedRelationStatement;
struct QueryStatement;

using Statement = std::variant<  //
    NamedRelationStatement, //
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
    ConstFieldBindingPtr field_binding;
};

}  // namespace lsql::ir

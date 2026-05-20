#pragma once

#include "core/types.h"
#include "iface/sql/bind/Relation.h"

#include <variant>
#include <vector>

namespace lsql::iface::sql::bind {

struct NamedRelationStatement;
struct QueryStatement;

using Statement = std::variant<  //
    NamedRelationStatement, //
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

}  // namespace lsql::iface::sql::bind

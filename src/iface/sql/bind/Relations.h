#pragma once

#include "iface/sql/bind/Context.h"

#include "iface/sql/ast/fwd/Relation.h"
#include "iface/sql/bound/fwd/Relation.h"

namespace lsql::iface::sql::bind {

bound::Relation bindRelation(ast::Relation rel, Context& ctx);

}  // namespace lsql::iface::sql::bind

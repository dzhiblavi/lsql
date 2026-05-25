#pragma once

#include "iface/sql/bind/Context.h"

#include "iface/sql/ast/Statement.h"
#include "iface/sql/bound/Statement.h"

namespace lsql::iface::sql::bind {

bound::Statement bindStatement(ast::Statement st, Context& ctx);

}  // namespace lsql::iface::sql::bind

#pragma once

#include "iface/sql/ast/Statement.h"
#include "ir/Statement.h"

namespace lsql::iface::sql::bind {

ir::Program bind(ast::Program program);

}  // namespace lsql::iface::sql::bind

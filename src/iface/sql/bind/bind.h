#pragma once

#include "iface/sql/bind/Statement.h"

namespace lsql::iface::sql::bind {

Program bind(ast::Program program);

}  // namespace lsql::iface::sql::bind

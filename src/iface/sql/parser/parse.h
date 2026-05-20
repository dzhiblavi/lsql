#pragma once

#include "iface/sql/ast/Statement.h"

#include <istream>

namespace lsql::iface::sql::parse {

ast::Program parse(std::istream& is);

}  // namespace lsql::iface::sql::parse

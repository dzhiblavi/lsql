#pragma once

#include "interface/sql/ast/Node.h"

#include <istream>
#include <memory>

namespace lsql::iface::sql::parse {

std::unique_ptr<ast::Node> parse(std::istream& is);

}  // namespace lsql::iface::sql

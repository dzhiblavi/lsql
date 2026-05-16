#pragma once

#include "sql/ast/Node.h"

#include <istream>
#include <memory>

namespace lsql::sql::parse {

std::unique_ptr<ast::Node> parse(std::istream& is);

}  // namespace lsql::sql

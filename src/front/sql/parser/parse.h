#pragma once

#include "front/sql/ast/Statement.h"

#include <istream>

namespace lsql::front::sql::parse {

ast::Program parse(std::istream& is);

}  // namespace lsql::front::sql::parse

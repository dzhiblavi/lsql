#pragma once

#include "front/sql/ast/Statement.h"

#include <string>

namespace lsql::front::sql::parse {

ast::Program parse(std::string query);

}  // namespace lsql::front::sql::parse

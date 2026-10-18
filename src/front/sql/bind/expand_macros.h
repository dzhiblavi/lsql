#pragma once

#include "front/sql/ast/Statement.h"

namespace lsql::front::sql::bind {

ast::Program expandMacros(ast::Program program);

}  // namespace lsql::front::sql::bind

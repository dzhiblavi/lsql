#pragma once

#include "front/sql/ast/Statement.h"
#include "front/sql/bound/Statement.h"

namespace lsql::front::sql::bind {

bound::Program bind(ast::Program program);

}  // namespace lsql::front::sql::bind

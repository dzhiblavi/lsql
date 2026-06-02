#pragma once

#include "front/sql/bind/Context.h"

#include "front/sql/ast/Statement.h"
#include "front/sql/bound/Statement.h"

namespace lsql::front::sql::bind {

bound::Statement bindStatement(ast::Statement st, Context& ctx);

}  // namespace lsql::front::sql::bind

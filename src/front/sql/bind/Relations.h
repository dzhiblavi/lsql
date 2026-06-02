#pragma once

#include "front/sql/bind/Context.h"

#include "front/sql/ast/fwd/Relation.h"
#include "front/sql/bound/fwd/Relation.h"

namespace lsql::front::sql::bind {

bound::Relation bindRelation(ast::Relation rel, Context& ctx);

}  // namespace lsql::front::sql::bind

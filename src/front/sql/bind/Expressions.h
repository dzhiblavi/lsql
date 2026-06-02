#pragma once

#include "front/sql/bind/Context.h"

#include "front/sql/ast/fwd/Expr.h"
#include "front/sql/bound/fwd/Expr.h"

namespace lsql::front::sql::bind {

bound::Expr bindExpr(ast::Expr expr, Context& ctx);

std::vector<bound::Expr> bindExprs(std::vector<ast::Expr> exprs, Context& ctx);

}  // namespace lsql::front::sql::bind

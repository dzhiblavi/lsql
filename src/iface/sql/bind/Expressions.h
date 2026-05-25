#pragma once

#include "iface/sql/bind/Context.h"

#include "iface/sql/ast/fwd/Expr.h"
#include "iface/sql/bound/fwd/Expr.h"

namespace lsql::iface::sql::bind {

bound::Expr bindExpr(ast::Expr expr, Context& ctx);

std::vector<bound::Expr> bindExprs(std::vector<ast::Expr> exprs, Context& ctx);

}  // namespace lsql::iface::sql::bind

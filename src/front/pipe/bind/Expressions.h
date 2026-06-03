#pragma once

#include "front/pipe/bind/Context.h"

#include "front/pipe/ast/fwd/Expr.h"
#include "front/pipe/bound/fwd/Expr.h"

namespace lsql::front::pipe::bind {

bound::Expr bindExpr(ast::Expr expr, Context& ctx);

std::vector<bound::Expr> bindExprs(std::vector<ast::Expr> exprs, Context& ctx);

}  // namespace lsql::front::pipe::bind

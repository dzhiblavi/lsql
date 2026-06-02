#pragma once

#include "front/sql/lower/Context.h"

#include "front/sql/bound/Expressions.h"

#include "ir/Aggregate.h"
#include "ir/Scalar.h"

namespace lsql::front::sql::lower {

using LowerExprResult = std::pair<ir::Scalar, std::vector<ir::Aggregate>>;
using LowerExprsResult = std::pair<std::vector<ir::Scalar>, std::vector<ir::Aggregate>>;

LowerExprResult lowerToIR(bound::Expr expr, Context& ctx);
LowerExprsResult lowerToIR(std::vector<bound::Expr> exprs, Context& ctx);

}  // namespace lsql::front::sql::lower

#pragma once

#include "front/pipe/lower/Context.h"

#include "front/pipe/bound/Expressions.h"

#include "ir/Aggregate.h"
#include "ir/Scalar.h"

namespace lsql::front::pipe::lower {

using LowerExprResult = std::pair<ir::Scalar, std::vector<ir::Aggregate>>;
using LowerExprsResult = std::pair<std::vector<ir::Scalar>, std::vector<ir::Aggregate>>;

LowerExprResult lowerToIR(bound::Expr expr, Context& ctx);
LowerExprsResult lowerToIR(std::vector<bound::Expr> exprs, Context& ctx);

}  // namespace lsql::front::pipe::lower

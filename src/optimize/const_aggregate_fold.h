#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program constAggregateFold(ir::Program program, Context& ctx);

}  // namespace lsql::opt

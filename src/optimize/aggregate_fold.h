#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program aggregateFold(ir::Program program, Context& ctx);

}  // namespace lsql::opt

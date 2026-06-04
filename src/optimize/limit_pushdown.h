#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program limitPushdown(ir::Program program, Context& ctx);

}  // namespace lsql::opt

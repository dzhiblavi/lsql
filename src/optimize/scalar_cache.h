#pragma once

#include "optimize/optimize.h"

namespace lsql::opt {

ir::Program scalarCache(ir::Program program, Context& ctx);

}  // namespace lsql::opt

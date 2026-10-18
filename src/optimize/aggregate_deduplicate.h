#pragma once

#include "optimize/optimize.h"

namespace lsql::opt {

ir::Program aggregateDeduplicate(ir::Program program, Context& ctx);

}  // namespace lsql::opt

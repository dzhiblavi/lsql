#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program relationSimplify(ir::Program program, Context& ctx);

}  // namespace lsql::opt

#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program constScalarFold(ir::Program program, Context& ctx);

}  // namespace lsql::opt

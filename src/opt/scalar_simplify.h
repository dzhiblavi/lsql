#pragma once

#include "opt/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program scalarSimplify(ir::Program program, Context& ctx);

}  // namespace lsql::opt

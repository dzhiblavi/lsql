#pragma once

#include "opt/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program constFold(ir::Program program, Context& ctx);

}  // namespace lsql::opt

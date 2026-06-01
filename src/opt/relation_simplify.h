#pragma once

#include "opt/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program relationSimplify(ir::Program program, Context& ctx);

}  // namespace lsql::exec

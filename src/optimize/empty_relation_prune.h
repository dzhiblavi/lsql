#pragma once

#include "optimize/optimize.h"

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program emptyRelationPrune(ir::Program program, Context& ctx);

}  // namespace lsql::opt

#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

// Single pass
ir::Program optimize(ir::Program program);

}  // namespace lsql::opt

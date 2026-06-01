#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program projectionCollapse(ir::Program program);

}  // namespace lsql::exec

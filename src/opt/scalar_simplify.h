#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program scalarSimplify(ir::Program program);

}  // namespace lsql::exec

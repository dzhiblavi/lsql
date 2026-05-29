#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program relationSimplify(ir::Program program);

}  // namespace lsql::exec

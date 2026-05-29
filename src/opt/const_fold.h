#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program constFold(ir::Program program);

}  // namespace lsql::exec

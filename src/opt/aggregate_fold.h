#pragma once

#include "ir/Statement.h"

namespace lsql::opt {

ir::Program aggregateFold(ir::Program program);

}  // namespace lsql::exec

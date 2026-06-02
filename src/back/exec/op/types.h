#pragma once

#include "back/exec/expr/Scalar.h"

#include <vector>

namespace lsql::back::exec {

using SortList = std::vector<ScalarPtr>;
using SortKey = std::vector<Value>;

}  // namespace lsql::back::exec

#pragma once

#include "exec/expr/Scalar.h"

#include <vector>

namespace lsql::exec {

using SortList = std::vector<ScalarPtr>;
using SortKey = std::vector<Value>;

}  // namespace lsql::exec

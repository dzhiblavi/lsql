#pragma once

#include "exec/expr/Expression.h"

#include <vector>

namespace lsql::exec {

using SortList = std::vector<ExpressionPtr>;
using SortKey = std::vector<Value>;

}  // namespace lsql::exec

#pragma once

#include "front/sql/bound/Statement.h"

#include "ir/Statement.h"

namespace lsql::front::sql::lower {

ir::Program lowerToIR(bound::Program program);

}  // namespace lsql::front::sql::lower

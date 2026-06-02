#pragma once

#include "ir/Statement.h"

#include "front/sql/bound/Statement.h"
#include "front/sql/lower/Context.h"

namespace lsql::front::sql::lower {

ir::Statement lowerToIR(bound::Statement st, Context& ctx);

}  // namespace lsql::front::sql::lower

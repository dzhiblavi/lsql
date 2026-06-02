#pragma once

#include "front/sql/lower/Context.h"

#include "front/sql/bound/Relations.h"

namespace lsql::front::sql::lower {

ir::Relation lowerToIR(bound::Relation expr, Context& ctx);

}  // namespace lsql::front::sql::lower

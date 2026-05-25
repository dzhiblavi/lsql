#pragma once

#include "iface/sql/lower/Context.h"

#include "iface/sql/bound/Relations.h"

namespace lsql::iface::sql::lower {

ir::Relation lowerToIR(bound::Relation expr, Context& ctx);

}  // namespace lsql::iface::sql::lower

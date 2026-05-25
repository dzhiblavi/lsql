#pragma once

#include "ir/Statement.h"

#include "iface/sql/bound/Statement.h"
#include "iface/sql/lower/Context.h"

namespace lsql::iface::sql::lower {

ir::Statement lowerToIR(bound::Statement st, Context& ctx);

}  // namespace lsql::iface::sql::lower

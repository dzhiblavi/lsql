#pragma once

#include "iface/sql/bound/Statement.h"

#include "ir/Statement.h"

namespace lsql::iface::sql::lower {

ir::Program lowerToIR(bound::Program program);

}  // namespace lsql::iface::sql::lower

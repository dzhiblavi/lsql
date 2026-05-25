#pragma once

#include "iface/sql/bind/Statement.h"

#include "ir/Statement.h"

namespace lsql::iface::sql::lower {

ir::Program lowerToIR(bind::Program program);

}  // namespace lsql::iface::sql::lower

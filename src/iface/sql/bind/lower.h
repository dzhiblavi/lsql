#pragma once

#include "iface/sql/bind/Statement.h"

#include "ir/Statement.h"

namespace lsql::iface::sql::bind {

ir::Program lowerToIR(Program program);

}  // namespace lsql::iface::sql::bind

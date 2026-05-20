#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Source.h"

#include "interface/sql/bind/Statement.h"

#include <vector>

namespace lsql::iface::sql::exe {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<exec::OperationPtr> top_operations;
};

Plan plan(bind::Program program);

}  // namespace lsql::iface::sql::exe

#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Source.h"

#include "ir/Statement.h"

#include <vector>

namespace lsql::exec {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<exec::OperationPtr> top_operations;
    ConstFieldBindingPtr field_binding;
};

Plan plan(ir::Program program);

}  // namespace lsql::iface::sql::exe

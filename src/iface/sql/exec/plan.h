#pragma once

#include "exec/op/Operation.h"
#include "exec/op/Source.h"

#include "iface/sql/bind/bind.h"

#include <vector>

namespace lsql::iface::sql::exe {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<exec::OperationPtr> top_operations;
    ConstFieldBindingPtr field_binding;
};

Plan plan(bind::BoundProgram program);

}  // namespace lsql::iface::sql::exe

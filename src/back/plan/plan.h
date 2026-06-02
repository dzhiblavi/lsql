#pragma once

#include "back/exec/op/Operation.h"
#include "back/exec/op/Source.h"

#include "ir/Statement.h"

#include <vector>

namespace lsql::back::plan {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<std::pair<exec::OperationPtr, FieldSet>> top_operations;
    ConstFieldBindingPtr field_binding;
};

Plan plan(ir::Program program);

}  // namespace lsql::back::plan

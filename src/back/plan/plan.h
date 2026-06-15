#pragma once

#include "back/plan/TimeRange.h"

#include "back/exec/op/Operation.h"
#include "back/exec/op/Source.h"

#include "ir/Statement.h"

#include <vector>

namespace lsql::back::plan {

struct Plan {
    std::vector<exec::SourcePtr> sources;
    std::vector<std::pair<exec::OperationPtr, Schema>> top_operations;
    ConstFieldBindingPtr field_binding;
};

struct Settings {
    std::optional<TimeRange> default_time_range;
};

Plan plan(ir::Program program, Settings settings);

}  // namespace lsql::back::plan

#pragma once

#include "back/exec/plan/Operation.h"

#include "ir/Statement.h"

#include <optional>

namespace lsql::back::exec::plan {

struct Settings {
    std::optional<TimeRange> default_time_range;
};

struct Plan {
    std::vector<Arc<Operation>> top_operations;
    ConstFieldBindingPtr field_binding;
};

Plan plan(ir::Program program, Settings settings);

}  // namespace lsql::back::exec::plan

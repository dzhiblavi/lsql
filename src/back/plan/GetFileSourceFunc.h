#pragma once

#include "back/exec/op/Source.h"

#include "core/Fields.h"
#include "core/types.h"

#include <functional>
#include <string>

namespace lsql::back::plan {

struct TimeRange {
    timestamp_t ts_from;
    timestamp_t ts_to;
};

using GetFileSourceFuncType = std::function<back::exec::SourcePtr(
    std::string, ConstFieldBindingPtr, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

}  // namespace lsql::back::exec

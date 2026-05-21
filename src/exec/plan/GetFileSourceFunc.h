#pragma once

#include "exec/op/Source.h"

#include "core/Fields.h"
#include "core/types.h"

#include <functional>
#include <string>

namespace lsql::exec {

struct TimeRange {
    timestamp_t ts_from;
    timestamp_t ts_to;
};

using GetFileSourceFuncType =
    std::function<exec::SourcePtr(std::string, ConstFieldBindingPtr, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

}  // namespace lsql::exec

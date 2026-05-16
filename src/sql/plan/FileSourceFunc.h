#pragma once

#include "exec/op/Source.h"

#include <functional>

namespace lsql::sql::plan {

struct TimeRange {
    std::string ts_from;
    int interval_s;
};

using GetFileSourceFuncType = std::function<exec::SourcePtr(std::string, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

}  // namespace lsql::sql::plan

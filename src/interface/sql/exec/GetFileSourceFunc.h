#pragma once

#include "exec/op/Source.h"

#include "core/types.h"

#include <functional>
#include <string>

namespace lsql::iface::sql::exe {

struct TimeRange {
    timestamp_t ts_from;
    timestamp_t ts_to;
};

using GetFileSourceFuncType = std::function<exec::SourcePtr(std::string, std::optional<TimeRange>)>;

GetFileSourceFuncType defaultFileSourceFunc();

}  // namespace lsql::iface::sql::exe

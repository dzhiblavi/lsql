#pragma once

#include "core/types.h"

namespace lsql::back::plan {

struct TimeRange {
    timestamp_t ts_from;
    timestamp_t ts_to;
};

}  // namespace lsql::back::plan

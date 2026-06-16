#pragma once

#include "back/storage/PagedFile.h"

#include "core/time_formats.h"
#include "core/types.h"

#include <cstddef>

namespace lsql::back::plan::search {

// index of the first character of the first line with ts >= x
size_t lowerBoundLine(const back::storage::File& file, timestamp_t ts, TimeFormat format);

// index of the first character of the first line with ts > x
size_t upperBoundLine(const back::storage::File& file, timestamp_t ts, TimeFormat format);

}  // namespace lsql::back::plan::search

#pragma once

#include "core/types.h"
#include "data/PagedFile.h"
#include "core/time_formats.h"

#include <cstddef>

namespace lsql::exec {

// index of the first character of the first line with ts >= x
size_t lowerBoundLine(const data::PagedFile& file, timestamp_t ts, TimeFormat format);

// index of the first character of the first line with ts > x
size_t upperBoundLine(const data::PagedFile& file, timestamp_t ts, TimeFormat format);

}  // namespace lsql::exec

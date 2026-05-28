#pragma once

#include "core/time_formats.h"
#include "core/types.h"

namespace lsql {

template <TimeFormat F>
std::string_view timeFormatRegex();

template <TimeFormat F>
timestamp_t timestampFromString(std::string_view s);

}  // namespace lsql

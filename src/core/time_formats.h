#pragma once

#include "core/types.h"

#include <reflex/pattern.h>

#include <string_view>

namespace lsql {

enum class TimeFormat {
    SQL,      // 2026-05-06 12:00:00
    ORACLE,   // 2026-May-06 12:00:00
    ISO8601,  // 2026-05-06T12:00:00
};

std::string_view timeFormatRegex(TimeFormat format);
const reflex::Pattern& timeFormatPattern(TimeFormat format);
timestamp_t timestampFromString(std::string_view s, TimeFormat format);

}  // namespace lsql

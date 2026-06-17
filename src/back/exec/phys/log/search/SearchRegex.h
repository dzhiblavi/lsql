#pragma once

#include "core/time_formats.h"
#include "core/types.h"

#include <optional>
#include <string_view>

namespace lsql::back::exec::phys {

std::optional<std::string_view> findFirst(std::string_view s, const reflex::Pattern& pattern);
std::optional<timestamp_t> searchFirstTimestamp(std::string_view s, TimeFormat format);

}  // namespace lsql::back::exec::phys

#pragma once

#include "core/time_formats.h"
#include "core/types.h"

#include <optional>
#include <string_view>

namespace lsql::logs {

std::optional<std::string_view> findFirst(std::string_view s, const reflex::Pattern& pattern);
std::optional<std::string_view> findLast(std::string_view s, const reflex::Pattern& rev_pattern);

std::optional<timestamp_t> searchFirstTimestamp(std::string_view s, TimeFormat format);
std::optional<timestamp_t> searchLastTimestamp(std::string_view s, TimeFormat format);

}  // namespace lsql::exec

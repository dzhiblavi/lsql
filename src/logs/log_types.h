#pragma once

#include "core/time_formats.h"

#include <absl/container/flat_hash_map.h>

#include <string_view>

namespace lsql::logs {

enum class LogType {
    TSKV2,
    IMAP,
};

using ParseKeyValueFunc =
    void (*)(std::string_view, absl::flat_hash_map<std::string_view, std::string_view>&);

ParseKeyValueFunc parseKeyValueFunc(LogType type);

std::optional<LogType> detectLogType(std::string_view line);

TimeFormat timeFormat(LogType type);

}  // namespace lsql::logs

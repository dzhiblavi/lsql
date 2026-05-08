#pragma once

#include "core/time_formats.h"

#include <string_view>
#include <unordered_map>

namespace lsql::logs {

enum class LogType {
    TSKV2,
    IMAP,
};

void parseKeyValue(
    std::string_view line,
    LogType type,
    std::unordered_map<std::string_view, std::string_view>& out);

std::optional<LogType> detectLogType(std::string_view line);

TimeFormat timeFormat(LogType type);

}  // namespace lsql::logs

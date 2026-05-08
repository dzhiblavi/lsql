#pragma once

#include "logs/log_types.h"

namespace lsql::logs {

template <LogType Type>
void parseKeyValue(
    std::string_view line, std::unordered_map<std::string_view, std::string_view>& out);

template <LogType Type>
bool detectLogType(std::string_view line);

template <LogType Type>
TimeFormat time_format;

}  // namespace lsql::logs

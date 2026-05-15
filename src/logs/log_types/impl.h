#pragma once

#include "logs/log_types.h"

namespace lsql::logs {

static constexpr size_t ExpectedKeysCountTunable = 32;

template <LogType Type>
void parseKeyValue(
    std::string_view line, absl::flat_hash_map<std::string_view, std::string_view>& out);

template <LogType Type>
bool detectLogType(std::string_view line);

template <LogType Type>
TimeFormat time_format;

}  // namespace lsql::logs

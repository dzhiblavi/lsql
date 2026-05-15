#pragma once

#include "core/time_formats.h"
#include "logs/log_types/impl.h"
#include "util/enum.h"

#include "logs/log_types/imap.h"
#include "logs/log_types/tskv2.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <string_view>

namespace lsql::logs {

template <typename F>
using ParseKeyValueFunc = void (*)(std::string_view, F&& callback);

template <typename F>
ParseKeyValueFunc<F> parseKeyValueFunc(LogType type) {
    static const std::array<ParseKeyValueFunc<F>, magic_enum::enum_count<LogType>()> funcs =
        util::enum_apply<LogType>([]<LogType... T> {
            return std::array{&LogTypeImpl<T>::template parseKeyValue<F&&>...};
        });

    return funcs[*magic_enum::enum_index(type)];
}

std::optional<LogType> detectLogType(std::string_view line);

TimeFormat timeFormat(LogType type);

}  // namespace lsql::logs

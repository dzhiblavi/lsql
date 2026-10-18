#pragma once

#include "back/logfmt/log_types/impl.h"
#include "core/time_formats.h"
#include "util/enum.h"

#include "back/logfmt/log_types/csv.h"
#include "back/logfmt/log_types/jsonl.h"
#include "back/logfmt/log_types/ts_logfmt.h"
#include "back/logfmt/log_types/tskv.h"
#include "back/logfmt/log_types/tsv.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <array>
#include <string_view>

namespace lsql::back::logfmt {

template <typename F>
using ParseKeyValueFunc = bool (*)(std::string_view, F&& callback);

template <typename F>
ParseKeyValueFunc<F> parseKeyValueFunc(LogType type) {
    static const std::array<ParseKeyValueFunc<F>, magic_enum::enum_count<LogType>()> funcs =
        util::enum_apply<LogType>([]<LogType... T> {
            return std::array{&LogTypeImpl<T>::template parseKeyValue<F&&>...};
        });

    return funcs[*magic_enum::enum_index(type)];
}

std::optional<LogType> detectLogType(std::string_view line);

std::optional<TimeFormat> timeFormat(LogType type);

}  // namespace lsql::back::logfmt

#include "back/logfmt/log_types.h"
#include "util/enum.h"

namespace lsql::back::logfmt {

std::optional<LogType> detectLogType(std::string_view line) {
    std::optional<LogType> result;

    auto test = [&]<LogType T> {
        if (LogTypeImpl<T>::detectLogType(line)) {
            result = T;
            return true;
        }

        return false;
    };

    util::enum_apply<LogType>([&]<LogType... F> { (test.template operator()<F>() || ...); });
    return result;
}

std::optional<TimeFormat> timeFormat(LogType type) {
    static const std::array<std::optional<TimeFormat>, magic_enum::enum_count<LogType>()> formats =
        util::enum_apply<LogType>([]<LogType... F> {
            return std::array{LogTypeImpl<F>::time_format...};
        });

    return formats[*magic_enum::enum_index(type)];
}

}  // namespace lsql::back::logfmt

#include "logs/log_types.h"
#include "util/enum.h"

namespace lsql::logs {

template <LogType Type>
void parseKeyValue(
    std::string_view line, std::unordered_map<std::string_view, std::string_view>& out);

template <LogType Type>
bool detectLogType(std::string_view line);

template <LogType Type>
TimeFormat time_format;

void parseKeyValue(
    std::string_view line,
    LogType type,
    std::unordered_map<std::string_view, std::string_view>& out) {
    using parse_func =
        void (*)(std::string_view, std::unordered_map<std::string_view, std::string_view>&);

    static const std::array<parse_func, magic_enum::enum_count<LogType>()> funcs =
        util::enum_apply<LogType>([]<LogType... F> { return std::array{&parseKeyValue<F>...}; });

    return funcs[*magic_enum::enum_index(type)](line, out);
}

std::optional<LogType> detectLogType(std::string_view line) {
    std::optional<LogType> result;

    auto test = [&]<LogType T> {
        if (detectLogType<T>(line)) {
            result = T;
            return true;
        }

        return false;
    };

    util::enum_apply<LogType>([&]<LogType... F> { (test.template operator()<F>() || ...); });
    return result;
}

TimeFormat timeFormat(LogType type) {
    static const std::array<TimeFormat, magic_enum::enum_count<LogType>()> formats =
        util::enum_apply<LogType>([]<LogType... F> { return std::array{time_format<F>...}; });

    return formats[*magic_enum::enum_index(type)];
}

}  // namespace lsql::logs

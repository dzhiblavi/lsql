#include "core/time_formats.h"
#include "util/enum.h"

#include <cassert>

namespace lsql {

template <TimeFormat F>
std::string_view timeFormatRegex();

template <TimeFormat F>
timestamp_t timestampFromString(std::string_view s);

std::string_view timeFormatRegex(TimeFormat format) {
    static const std::array<std::string_view, magic_enum::enum_count<TimeFormat>()> regexes =
        util::enum_apply<TimeFormat>([]<TimeFormat... F>() {
            return std::array{timeFormatRegex<F>()...};
        });

    return regexes[*magic_enum::enum_index(format)];
}

const reflex::Pattern& timeFormatPattern(TimeFormat format) {
    static const std::array<reflex::Pattern, magic_enum::enum_count<TimeFormat>()> regexes =
        util::enum_apply<TimeFormat>([]<TimeFormat... F> {
            return std::array{reflex::Pattern(timeFormatRegex<F>().data())...};
        });

    return regexes[*magic_enum::enum_index(format)];
}

timestamp_t timestampFromString(std::string_view s, TimeFormat format) {
    using conv_func = timestamp_t (*)(std::string_view);
    static const std::array<conv_func, magic_enum::enum_count<TimeFormat>()> funcs =
        util::enum_apply<TimeFormat>([]<TimeFormat... F> {
            return std::array{&timestampFromString<F>...};
        });

    return funcs[*magic_enum::enum_index(format)](s);
}

}  // namespace lsql

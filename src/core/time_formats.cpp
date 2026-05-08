#include "core/time_formats.h"
#include "util/enum.h"

#include <algorithm>

#include <cassert>
#include <regex>

namespace lsql {

template <TimeFormat F>
std::string_view timeFormatRegex();

template <TimeFormat F>
timestamp_t timestampFromString(std::string_view s);

namespace {

using raw_type = std::underlying_type_t<TimeFormat>;

std::string reverseRegex(std::string_view sv) {
    std::string f(sv);
    {
        std::regex pattern(R"(\\d\{(\d{1})\})");
        f = std::regex_replace(f, pattern, R"(}$1{d\)");
    }
    {
        std::regex pattern(R"(\\w\{(\d{1})\})");
        f = std::regex_replace(f, pattern, R"(}$1{w\)");
    }
    std::ranges::reverse(f);
    return f;
};

}  // namespace

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

const reflex::Pattern& timeFormatReversePattern(TimeFormat format) {
    static const std::array<reflex::Pattern, magic_enum::enum_count<TimeFormat>()> regexes =
        util::enum_apply<TimeFormat>([]<TimeFormat... F> {
            return std::array{reflex::Pattern(reverseRegex(timeFormatRegex<F>()))...};
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

#include "core/time_formats.h"
#include "util/enum.h"

#include <cassert>

namespace lsql {

template <TimeFormat F>
std::string_view timeFormatRegex();

template <TimeFormat F>
timestamp_t timestampFromString(std::string_view s);

std::string_view timeFormatRegex(TimeFormat format) {
    return util::enum_dispatch([]<TimeFormat F> { return timeFormatRegex<F>(); }, format);
}

const reflex::Pattern& timeFormatPattern(TimeFormat format) {
    return util::enum_dispatch(
        []<TimeFormat F>() -> const reflex::Pattern& {
            static const reflex::Pattern pattern(timeFormatRegex<F>().data());
            return pattern;
        },
        format);
}

timestamp_t timestampFromString(std::string_view s, TimeFormat format) {
    auto conv = util::enum_dispatch([]<TimeFormat F> { return &timestampFromString<F>; }, format);
    return conv(s);
}

}  // namespace lsql

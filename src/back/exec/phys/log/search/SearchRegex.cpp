#include "back/exec/phys/log/search/SearchRegex.h"

#include <reflex/matcher.h>

namespace lsql::back::exec::phys {

std::optional<std::string_view> findFirst(std::string_view s, const reflex::Pattern& pattern) {
    reflex::Matcher matcher(&pattern, reflex::Input(s.data(), s.size()));

    if (!matcher.find()) {
        return std::nullopt;
    }

    return s.substr(matcher.first(), matcher.size());
}

std::optional<timestamp_t> searchFirstTimestamp(std::string_view s, TimeFormat format) {
    return findFirst(s, timeFormatPattern(format)).transform([&](std::string_view sv) {
        return timestampFromString(sv, format);
    });
}

}  // namespace lsql::back::exec::phys

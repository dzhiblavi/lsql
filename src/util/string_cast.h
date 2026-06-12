#pragma once

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string_view>

namespace lsql::util {

inline std::optional<int64_t> parseInt64Strict(std::string_view s) {
    int64_t value{};
    auto* begin = s.data();
    auto* end = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return value;
}

inline std::optional<float> parseFloatStrict(std::string_view s) {
    char* end = nullptr;
    errno = 0;

    // TODO: may read past s.end()
    float value = std::strtof(s.data(), &end);

    if (end != s.data() + s.size()) {  // NOLINT
        return std::nullopt;
    }

    if (errno == ERANGE) {
        return std::nullopt;
    }

    if (!std::isfinite(value)) {
        return std::nullopt;
    }

    return value;
}

}  // namespace lsql::util

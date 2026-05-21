#pragma once

#include <format>

namespace lsql {

template <typename... Args>
[[noreturn]] void throwError(std::format_string<const Args&...> fmt, const Args&... args) {
    throw std::runtime_error(std::format(fmt, args...));
}

template <typename... Args>
void require(bool value, std::format_string<const Args&...> fmt, const Args&... args) {
    if (value) [[likely]] {
        return;
    }

    throwError(fmt, args...);
}

}  // namespace lsql

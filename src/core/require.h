#pragma once

#include <format>

namespace lsql::iface::sql::bind {

template <typename... Args>
void require(bool value, std::format_string<const Args&...> fmt, const Args&... args) {
    if (value) [[likely]] {
        return;
    }

    throw std::runtime_error(std::format(fmt, args...));
}

}  // namespace lsql::iface::sql::bind

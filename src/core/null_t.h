#pragma once

#include <compare>  // IWYU pragma: keep
#include <utility>  // IWYU pragma: keep

namespace lsql {

struct null_t {
    auto operator<=>(const null_t&) const = default;
};

[[maybe_unused]] static constexpr null_t null{};

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::null_t> {
    size_t operator()(const lsql::null_t&) const noexcept { return 0x9e3779b9; }
};

}  // namespace std

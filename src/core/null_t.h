#pragma once

#include <compare>  // IWYU pragma: keep

namespace lsql {

struct null_t {
    auto operator<=>(const null_t&) const = default;
};

[[maybe_unused]] static constexpr null_t null{};

}  // namespace lsql

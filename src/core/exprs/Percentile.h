#pragma once

#include "core/exprs/concepts.h"

#include <string>

namespace lsql {

struct PercentileTraits {
    template <typename T>
    static constexpr bool allowed() {
        return Comparable<T>;
    }

    template <typename T>
    using ValueType = std::string;
};

}  // namespace lsql

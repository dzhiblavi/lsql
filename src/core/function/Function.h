#pragma once

#include "core/value/ValueType.h"
#include "util/overloaded.h"

#include <cstddef>
#include <variant>
#include <vector>

namespace lsql::func {

struct Substr {
    size_t from;
    size_t length;

    bool operator==(const Substr&) const = default;
};

struct Coalesce {
    bool operator==(const Coalesce&) const = default;
};

struct RSubstr {
    std::string regex;

    bool operator==(const RSubstr&) const = default;
};

struct Like {
    std::string regex;

    bool operator==(const Like&) const = default;
};

struct Cast {
    ValueType cast_to;

    bool operator==(const Cast&) const = default;
};

struct Percentile {
    std::vector<float> percentiles;
    ValueType args_type;

    bool operator==(const Percentile&) const = default;
};

struct CountNonNull {
    bool operator==(const CountNonNull&) const = default;
};

struct CountAll {
    bool operator==(const CountAll&) const = default;
};

struct Min {
    ValueType arg_type;

    bool operator==(const Min&) const = default;
};

struct Max {
    ValueType arg_type;

    bool operator==(const Max&) const = default;
};

struct Sum {
    ValueType arg_type;

    bool operator==(const Sum&) const = default;
};

using Function = std::variant< //
    Substr, //
    Coalesce, //
    RSubstr, //
    Like, //
    Cast, //
    Percentile, //
    CountNonNull, //
    CountAll, //
    Min, //
    Max, //
    Sum //
>;

inline bool isScalar(const Function& f) {
    return util::match(
        f,
        util::Overloaded{
            [](const Substr&) { return true; },
            [](const Coalesce&) { return true; },
            [](const RSubstr&) { return true; },
            [](const Like&) { return true; },
            [](const Cast&) { return true; },
            [](const Percentile&) { return false; },
            [](const CountNonNull&) { return false; },
            [](const CountAll&) { return false; },
            [](const Min&) { return false; },
            [](const Max&) { return false; },
            [](const Sum&) { return false; },
        });
}

}  // namespace lsql::func

#pragma once

#include "core/time_formats.h"
#include "core/value/ValueType.h"
#include "util/overloaded.h"

#include <cstddef>
#include <variant>
#include <vector>

namespace lsql::func {

struct Lower {
    bool operator==(const Lower&) const = default;
};

struct SplitPart {
    char separator;
    size_t index;

    bool operator==(const SplitPart&) const = default;
};

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

struct ParseTimestamp {
    TimeFormat format;

    bool operator==(const ParseTimestamp&) const = default;
};

struct BooleanNegate {
    bool operator==(const BooleanNegate&) const = default;
};

struct Equal {
    bool operator==(const Equal&) const = default;
};

struct NotEqual {
    bool operator==(const NotEqual&) const = default;
};

struct And {
    bool operator==(const And&) const = default;
};

struct Or {
    bool operator==(const Or&) const = default;
};

struct Divide {
    ValueType arg_type;

    bool operator==(const Divide&) const = default;
};

struct Add {
    ValueType arg_type;

    bool operator==(const Add&) const = default;
};

struct Subtract {
    ValueType arg_type;

    bool operator==(const Subtract&) const = default;
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
    Lower, //
    SplitPart, //
    Substr, //
    Coalesce, //
    RSubstr, //
    Like, //
    Cast, //
    ParseTimestamp, //
    BooleanNegate, //
    Equal, //
    NotEqual, //
    And, //
    Or, //
    Divide, //
    Add, //
    Subtract, //
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
            [](const Lower&) { return true; },
            [](const SplitPart&) { return true; },
            [](const Substr&) { return true; },
            [](const Coalesce&) { return true; },
            [](const RSubstr&) { return true; },
            [](const Like&) { return true; },
            [](const Cast&) { return true; },
            [](const ParseTimestamp&) { return true; },
            [](const BooleanNegate&) { return true; },
            [](const Equal&) { return true; },
            [](const NotEqual&) { return true; },
            [](const And&) { return true; },
            [](const Or&) { return true; },
            [](const Divide&) { return true; },
            [](const Add&) { return true; },
            [](const Subtract&) { return true; },
            [](const Percentile&) { return false; },
            [](const CountNonNull&) { return false; },
            [](const CountAll&) { return false; },
            [](const Min&) { return false; },
            [](const Max&) { return false; },
            [](const Sum&) { return false; },
        });
}

}  // namespace lsql::func

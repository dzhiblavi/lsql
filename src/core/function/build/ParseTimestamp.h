#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct ParseTimestampExecutor {
    Value execute(Value value) const {
        if (value == vnull) {
            return vnull;
        }

        return int64_t(timestampFromString(value.get<std::string_view>(), format));
    }

    TimeFormat format;
};

static_assert(UnaryExecutor<ParseTimestampExecutor>);

inline ParseTimestampExecutor build(const ParseTimestamp& s) {
    return ParseTimestampExecutor(s.format);
}

}  // namespace lsql::func

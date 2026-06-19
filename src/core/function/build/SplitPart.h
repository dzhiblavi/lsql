#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct SplitPartExecutor {
    Value execute(Value value) const {
        if (value == vnull) {
            return vnull;
        }

        auto curr = value.get<std::string_view>();
        size_t offset = 0;

        for (size_t i = 0; i < index; ++i) {
            auto sep = curr.find(separator);
            if (sep == std::string::npos) {
                return vnull;
            }

            offset += sep + 1;
            curr = curr.substr(sep + 1);
        }

        auto sep = curr.find(separator);
        return value.substr(offset, sep);
    }

    size_t index;
    char separator;
};

static_assert(UnaryExecutor<SplitPartExecutor>);

inline SplitPartExecutor build(const SplitPart& s) {
    return SplitPartExecutor{s.index, s.separator};
}

}  // namespace lsql::func

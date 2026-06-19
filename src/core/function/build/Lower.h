#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

#include <algorithm>
#include <cctype>

namespace lsql::func {

struct LowerExecutor {
    Value execute(Value value) const {
        if (value == vnull) {
            return vnull;
        }

        auto copy = std::string(value.get<std::string_view>());
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return Value(std::move(copy));
    }
};

static_assert(UnaryExecutor<LowerExecutor>);

inline LowerExecutor build(const Lower& /*s*/) {
    return {};
}

}  // namespace lsql::func

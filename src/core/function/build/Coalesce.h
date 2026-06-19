#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct CoalesceExecutor {
    Value execute(std::span<Value> values) const {
        for (auto&& value : values) {
            if (value != vnull) {
                return value;
            }
        }
        return vnull;
    }
};

static_assert(NaryExecutor<CoalesceExecutor>);

inline CoalesceExecutor build(const Coalesce& /*s*/) {
    return {};
}

}  // namespace lsql::func

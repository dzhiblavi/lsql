#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct OrExecutor {
    Value execute(const Value& a, const Value& b) const { return a.get<bool>() || b.get<bool>(); }
};

static_assert(BinaryExecutor<OrExecutor>);

inline OrExecutor build(const Or& /*s*/) {
    return {};
}

}  // namespace lsql::func

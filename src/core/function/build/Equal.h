#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct EqualExecutor {
    Value execute(const Value& a, const Value& b) const { return a == b; }
};

static_assert(BinaryExecutor<EqualExecutor>);

inline EqualExecutor build(const Equal& /*s*/) {
    return {};
}

}  // namespace lsql::func

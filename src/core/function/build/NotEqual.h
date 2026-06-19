#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct NotEqualExecutor {
    Value execute(const Value& l, const Value& r) const { return l != r; }
};

static_assert(BinaryExecutor<NotEqualExecutor>);

inline NotEqualExecutor build(const NotEqual& /*s*/) {
    return {};
}

}  // namespace lsql::func

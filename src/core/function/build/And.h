#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

struct AndExecutor {
    Value execute(const Value& a, const Value& b) const { return a.get<bool>() && b.get<bool>(); }
};

static_assert(BinaryExecutor<AndExecutor>);

inline AndExecutor build(const And& /*s*/) {
    return {};
}

}  // namespace lsql::func

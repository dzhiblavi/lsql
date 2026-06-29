#pragma once

#include "core/function/Concepts.h"
#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

template <Dividable T>
struct DivideExecutor {
    Value execute(const Value& l, const Value& r) const {
        auto divisor = r.get<T>();
        return divisor == T(0) ? null : Value(l.get<T>() / divisor);
    }
};

static_assert(BinaryExecutor<DivideExecutor<int64_t>>);

template <typename T>
inline DivideExecutor<T> build(const Divide& /*s*/) {
    return {};
}

}  // namespace lsql::func

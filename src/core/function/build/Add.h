#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

template <Addable T>
struct AddExecutor {
    using U = std::conditional_t<std::same_as<T, std::string_view>, std::string, T>;

    Value execute(const Value& l, const Value& r) const { return U(l.get<T>()) + U(r.get<T>()); }
};

static_assert(BinaryExecutor<AddExecutor<int64_t>>);

template <Addable T>
inline AddExecutor<T> build(const Add& /*s*/) {
    return {};
}

}  // namespace lsql::func

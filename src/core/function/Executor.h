#pragma once

#include "core/value/Value.h"

#include <span>

namespace lsql::func {

template <typename E>
concept Executor = requires(const E& e, std::span<Value> args) {
    { e.execute(args) } -> std::same_as<Value>;
};

}  // namespace lsql::func

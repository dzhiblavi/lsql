#pragma once

#include "core/value/Value.h"

#include <span>

namespace lsql::func {

template <typename E>
concept NaryExecutor = requires(const E& e, std::span<Value> args) {
    { e.execute(args) } -> std::same_as<Value>;
};

template <typename E>
concept UnaryExecutor = requires(const E& e, Value arg) {
    { e.execute(arg) } -> std::same_as<Value>;
};

template <typename E>
concept BinaryExecutor = requires(const E& e, Value arg) {
    { e.execute(arg, arg) } -> std::same_as<Value>;
};

template <NaryExecutor E>
Value execute(const E& e, std::span<Value> values) {
    return e.execute(values);
}

template <UnaryExecutor E>
Value execute(const E& e, std::span<Value> values) {
    verify(values.size() == 1);
    return e.execute(values[0]);
}

template <BinaryExecutor E>
Value execute(const E& e, std::span<Value> values) {
    verify(values.size() == 2);
    return e.execute(values[0], values[1]);
}

}  // namespace lsql::func

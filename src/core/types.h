#pragma once

#include <cstdint>
#include <memory>

namespace lsql {

using timestamp_t = int64_t;

template <typename T>
using Box = std::unique_ptr<T>;

template <typename T>
using Arc = std::shared_ptr<T>;

template <typename T, typename... Args>
Box<T> box(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
Box<T> box(T value) {
    return std::make_unique<T>(std::move(value));
}

template <typename T, typename... Args>
Arc<T> arc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
Arc<T> arc(T value) {
    return std::make_shared<T>(std::move(value));
}

}  // namespace lsql

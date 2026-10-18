#pragma once

#include "util/concepts.h"
#include "util/verify.h"

#include <rfl/to_view.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

namespace lsql::util {

inline void hashCombine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <StdHashable T>
std::size_t hash(const T& value);

template <typename T>
std::size_t hash(const std::unique_ptr<T>& value);

template <typename T>
std::size_t hash(const std::vector<T>& values);

template <typename... Ts>
std::size_t hash(const std::variant<Ts...>& value);

template <typename T>
requires(!StdHashable<T> && std::is_aggregate_v<T>)
std::size_t hash(const T& value);

template <StdHashable T>
std::size_t hash(const T& value) {
    return std::hash<T>{}(value);
}

template <typename T>
std::size_t hash(const std::unique_ptr<T>& value) {
    verify(value != nullptr);
    return hash(*value);
}

template <typename T>
std::size_t hash(const std::vector<T>& values) {
    std::size_t seed = values.size();
    for (const T& value : values) {
        hashCombine(seed, hash(value));
    }
    return seed;
}

template <typename... Ts>
std::size_t hash(const std::variant<Ts...>& value) {
    std::size_t seed = value.index();
    std::visit([&](const auto& node) { hashCombine(seed, hash(node)); }, value);
    return seed;
}

template <typename T>
requires(!StdHashable<T> && std::is_aggregate_v<T>)
std::size_t hash(const T& value) {
    std::size_t seed = 0;
    rfl::to_view(value).apply([&](const auto& field) { hashCombine(seed, hash(*field.value())); });
    return seed;
}

}  // namespace lsql::util

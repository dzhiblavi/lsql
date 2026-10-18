#pragma once

#include <rfl/apply.hpp>
#include <rfl/to_view.hpp>

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace lsql::util {

template <typename T>
requires(!std::is_aggregate_v<T>)
T deepCopy(const T& value);

template <typename T>
std::unique_ptr<T> deepCopy(const std::unique_ptr<T>& value);

template <typename T>
std::vector<T> deepCopy(const std::vector<T>& values);

template <typename T>
std::optional<T> deepCopy(const std::optional<T>& value);

template <typename... Ts>
std::variant<Ts...> deepCopy(const std::variant<Ts...>& value);

template <typename T>
requires std::is_aggregate_v<T>
T deepCopy(const T& value);

template <typename T>
requires(!std::is_aggregate_v<T>)
T deepCopy(const T& value) {
    return value;
}

template <typename T>
std::unique_ptr<T> deepCopy(const std::unique_ptr<T>& value) {
    return value ? std::make_unique<T>(deepCopy(*value)) : nullptr;
}

template <typename T>
std::vector<T> deepCopy(const std::vector<T>& values) {
    std::vector<T> result;
    result.reserve(values.size());
    for (const auto& value : values) {
        result.push_back(deepCopy(value));
    }
    return result;
}

template <typename T>
std::optional<T> deepCopy(const std::optional<T>& value) {
    return value ? std::optional<T>(deepCopy(*value)) : std::nullopt;
}

template <typename... Ts>
std::variant<Ts...> deepCopy(const std::variant<Ts...>& value) {
    return std::visit(
        [](const auto& node) -> std::variant<Ts...> { return deepCopy(node); }, value);
}

template <typename T>
requires std::is_aggregate_v<T>
T deepCopy(const T& value) {
    return rfl::apply(
        [](const auto&... fields) { return T{deepCopy(*fields.value())...}; },
        rfl::to_view(value).fields());
}

}  // namespace lsql::util

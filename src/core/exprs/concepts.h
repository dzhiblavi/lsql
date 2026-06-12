#pragma once

#include <concepts>
#include <cstdint>
#include <string_view>

namespace lsql {

template <typename T>
concept Dividable = std::same_as<T, int64_t> || std::same_as<T, float>;

template <typename T>
concept Addable =
    std::same_as<T, int64_t> || std::same_as<T, float> || std::same_as<T, std::string_view>;

template <typename T>
concept Subtractable = std::same_as<T, int64_t> || std::same_as<T, float>;

template <typename T>
concept Comparable =
    std::same_as<T, int64_t> || std::same_as<T, float> || std::same_as<T, std::string_view>;

}  // namespace lsql

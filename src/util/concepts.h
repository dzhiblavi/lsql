#pragma once

#include <concepts>
#include <type_traits>

namespace lsql::util {

namespace detail {

template <typename U, template <typename> typename T>
struct IsInstanceOfTemplate : std::false_type {};

template <typename... Ts, template <typename> typename T>
struct IsInstanceOfTemplate<T<Ts...>, T> : std::true_type {};

}  // namespace detail

template <typename U, template <typename> typename T>
static constexpr bool IsInstanceOfTemplate = detail::IsInstanceOfTemplate<U, T>::value;

template <typename U, template <typename> typename T>
concept InstanceOf = IsInstanceOfTemplate<U, T>;

template <typename T>
concept EqualityComparable = requires(const T& l, const T& r) {
    { l == r } -> std::same_as<bool>;
};

template <typename T>
concept NotEqualityComparable = !EqualityComparable<T>;

template <typename E>
concept Enum = std::is_enum_v<E>;

}  // namespace lsql::util

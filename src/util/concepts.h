#pragma once

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

}  // namespace lsql::util

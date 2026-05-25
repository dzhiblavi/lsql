#pragma once

#include <magic_enum/magic_enum.hpp>

namespace lsql::util {

template <typename Enum, typename Func>
constexpr decltype(auto) enum_apply(Func&& func) {
    constexpr auto count = magic_enum::enum_count<Enum>();

    return [&]<size_t... Is>(std::index_sequence<Is...>) {
        return func.template operator()<magic_enum::enum_value<Enum>(Is)...>();
    }(std::make_index_sequence<count>{});
}

template <typename Enum, typename Func>
constexpr decltype(auto) enum_dispatch(Func&& func, Enum value) {
    using F = std::remove_reference_t<Func>;
    using R = decltype(std::declval<F&>().template operator()<magic_enum::enum_value<Enum>(0)>());
    using Fn = R (*)(F&);

    static constexpr std::array<Fn, magic_enum::enum_count<Enum>()> table =
        enum_apply<Enum>([]<Enum... E>() {
            return std::array<Fn, sizeof...(E)>{+[](F& f) -> R {
                return f.template operator()<E>();
            }...};
        });

    return table[magic_enum::enum_underlying(value)](func);
}

}  // namespace lsql::util

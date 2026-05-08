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

}  // namespace lsql::util

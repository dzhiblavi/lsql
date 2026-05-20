#pragma once

#include <variant>

namespace lsql::util {

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename V, typename... F>
decltype(auto) match(V&& variant, F&&... funcs) {
    return std::visit(Overloaded{std::forward<F>(funcs)...}, std::forward<V>(variant));
}

template <typename V, typename... F>
void matchPartial(V&& variant, F&&... funcs) {
    std::visit(Overloaded{std::forward<F>(funcs)..., [](auto&&...) {}}, std::forward<V>(variant));
}

}  // namespace lsql::util

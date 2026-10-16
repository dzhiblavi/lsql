#pragma once

#include "util/concepts.h"
#include "util/overloaded.h"
#include "util/verify.h"

#include <rfl/to_view.hpp>

#include <memory>
#include <ranges>
#include <variant>
#include <vector>

namespace lsql::util {

template <EqualityComparable T>
bool equal(const T& l, const T& r);

template <typename Node>
bool equal(const std::unique_ptr<Node>& l, const std::unique_ptr<Node>& r);

template <typename Node>
bool equal(const std::vector<Node>& l, const std::vector<Node>& r);

template <typename... Nodes>
bool equal(const std::variant<Nodes...>& l, const std::variant<Nodes...>& r);

template <NotEqualityComparable Node>
requires std::is_aggregate_v<Node>
bool equal(const Node& l, const Node& r);

template <EqualityComparable T>
bool equal(const T& l, const T& r) {
    return l == r;
}

template <typename Node>
bool equal(const std::unique_ptr<Node>& l, const std::unique_ptr<Node>& r) {
    verify(l != nullptr);
    verify(r != nullptr);
    return equal(*l, *r);
}

template <typename Node>
bool equal(const std::vector<Node>& l, const std::vector<Node>& r) {
    if (l.size() != r.size()) {
        return false;
    }

    for (auto&& [ls, rs] : std::views::zip(l, r)) {
        if (!equal(ls, rs)) {
            return false;
        }
    }

    return true;
}

template <typename... Nodes>
bool equal(const std::variant<Nodes...>& l, const std::variant<Nodes...>& r) {
    return std::visit(
        util::Overloaded{
            []<typename T>(const T& lv, const T& rv) { return equal(lv, rv); },
            [](auto&&...) { return false; },
        },
        l,
        r);
}

template <NotEqualityComparable Node>
requires std::is_aggregate_v<Node>
bool equal(const Node& l, const Node& r) {
    const auto view_lhs = rfl::to_view(l);
    const auto view_rhs = rfl::to_view(r);
    constexpr std::size_t num_fields = rfl::tuple_size_v<decltype(view_lhs.values())>;

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... && [&]() {
            const auto field_lhs = rfl::get<Is>(view_lhs);
            const auto field_rhs = rfl::get<Is>(view_rhs);
            return equal(*field_lhs, *field_rhs);
        }());
    }(std::make_index_sequence<num_fields>{});
}

}  // namespace lsql::util

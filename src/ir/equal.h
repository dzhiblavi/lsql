#pragma once

#include "ir/reflect.h"

#include "core/types.h"

#include <ranges>
#include <variant>
#include <vector>

namespace lsql::ir {

template <typename Node>
bool equal(const Box<Node>& l, const Box<Node>& r) {
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
    if (l.index() != r.index()) {
        return false;
    }

    return std::visit([&r]<typename T>(const T& lv) { return equal(lv, std::get<T>(r)); }, l);
}

template <Reflectable Node>
bool equal(const Node& l, const Node& r) {
    if (fields(l) != fields(r)) {
        return false;
    }

    return std::apply(
        [&](auto&&... ls) {
            return std::apply([&](auto&&... rs) { return (equal(ls, rs) && ...); }, childNodes(r));
        },
        childNodes(l));
}

}  // namespace lsql::ir

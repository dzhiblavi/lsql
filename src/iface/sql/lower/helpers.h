#pragma once

#include <vector>

namespace lsql::iface::sql::lower {

template <typename T>
void append(std::vector<T>& a, std::vector<T> b) {
    a.insert(a.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
}

template <typename T>
std::vector<T> concat(std::vector<T> a, std::vector<T> b) {
    append(a, std::move(b));
    return a;
}

}  // namespace lsql::iface::sql::lower

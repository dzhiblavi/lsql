#pragma once

#include <string_view>
#include <vector>

namespace lsql::front {

struct SourcePos {
    int line;
    int column;

    bool empty() const { return line == -1 && column == -1; }

    bool operator<(const SourcePos& o) const {
        return line != o.line ? line < o.line : column < o.column;
    }
};

struct SourceSpan {
    SourcePos begin;
    SourcePos end;

    bool empty() const { return begin.empty() || end.empty(); }
};

struct RichSourceSpan {
    std::string_view source;
    SourceSpan span;
};

inline SourceSpan merge(SourceSpan a, SourceSpan b) {
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }

    return {
        .begin = std::min(a.begin, b.begin),
        .end = std::max(a.end, b.end),
    };
}

template <typename T>
SourceSpan spanOf(const std::vector<T>& args) {
    auto span = SourceSpan({-1, -1}, {-1, -1});
    for (auto&& a : args) {
        span = merge(span, a.span);
    }
    return span;
}

}  // namespace lsql::front

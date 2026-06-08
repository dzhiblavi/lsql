#pragma once

#include <string_view>
#include <vector>

namespace lsql::front {

struct SourcePos {
    int line;
    int column;
    int offset;
};

struct SourceSpan {
    SourcePos begin;
    SourcePos end;
};

struct RichSourceSpan {
    std::string_view source;
    SourceSpan span;
};

inline SourceSpan merge(SourceSpan a, SourceSpan b) {
    return {
        .begin = a.begin,
        .end = b.end,
    };
}

template <typename T>
SourceSpan spanOf(const std::vector<T>& args) {
    if (args.empty()) {
        return SourceSpan{};
    }

    return merge(args.front().span, args.back().span);
}

}  // namespace lsql::front

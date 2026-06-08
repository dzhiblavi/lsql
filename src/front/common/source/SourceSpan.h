#pragma once

#include <string_view>

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

}  // namespace lsql::front

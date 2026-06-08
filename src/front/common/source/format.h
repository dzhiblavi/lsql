#pragma once

#include "front/common/source/SourceSpan.h"

#include <format>
#include <string>

namespace lsql::front {

inline std::string format(RichSourceSpan rich_span) {
    auto&& [source, span] = rich_span;

    auto lineBounds = [&](int offset) {
        int begin = offset;
        while (begin > 0 && source[begin - 1] != '\n')
            --begin;

        int end = offset;
        while (end < static_cast<int>(source.size()) && source[end] != '\n')
            ++end;

        return std::pair{begin, end};
    };

    auto lineStartOffset = [&](int line_no) {
        int line = 0;
        int offset = 0;

        while (offset < static_cast<int>(source.size()) && line < line_no) {
            if (source[offset++] == '\n')
                ++line;
        }

        return offset;
    };

    constexpr int context = 2;

    int first_line = std::max(0, span.begin.line - context);
    int last_line = span.end.line + context;

    int line_no_width = static_cast<int>(std::to_string(last_line).size());

    std::string out;
    out += std::format("line {}, column {}\n\n", span.begin.line, span.begin.column);

    int cursor = lineStartOffset(first_line);

    for (int line_no = first_line;
         line_no <= last_line && cursor <= static_cast<int>(source.size());
         ++line_no) {
        auto [lb, le] = lineBounds(cursor);
        auto line = source.substr(lb, le - lb);

        out += std::format("{:>{}} | {}\n", line_no, line_no_width, line);

        bool affected = line_no >= span.begin.line && line_no <= span.end.line;

        if (affected) {
            int caret_start = 0;
            int caret_width = static_cast<int>(line.size());

            if (line_no == span.begin.line) {
                caret_start = span.begin.column;
                caret_width = std::max(1, static_cast<int>(line.size()) - caret_start + 1);
            }

            if (line_no == span.end.line) {
                caret_width = std::max(1, span.end.column - caret_start);
            }

            out += std::format("{:>{}} | ", "", line_no_width);
            if (caret_start > 0) {
                out.append(caret_start - 1, ' ');
            }
            out.append(caret_width, '^');
            out += '\n';
        }

        cursor = le + 1;
    }

    return out;
}

}  // namespace lsql::front

#pragma once

#include "front/common/source/SourceSpan.h"

#include <cpptrace/exceptions.hpp>

#include <format>

namespace lsql::front {

class SpanRuntimeError : public cpptrace::exception_with_message {
 public:
    SpanRuntimeError(std::string message, SourceSpan span)
        : cpptrace::exception_with_message(std::move(message))
        , span_(span) {}

    SourceSpan span() const { return span_; }

 private:
    SourceSpan span_;
};

template <typename... Args>
[[noreturn]] void throwAt(
    SourceSpan span, std::format_string<const Args&...> fmt, const Args&... args) {
    throw SpanRuntimeError(std::format(fmt, args...), span);
}

template <typename... Args>
void requireAt(
    bool condition, SourceSpan span, std::format_string<const Args&...> fmt, const Args&... args) {
    if (condition) [[likely]] {
        return;
    }

    throwAt(span, fmt, args...);
}

}  // namespace lsql::front

#pragma once

#include "config/build_settings.h"

#include <cpptrace/exceptions.hpp>
#include <cpptrace/formatting.hpp>

#include <format>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace lsql {

using RuntimeError = std::conditional_t<
    config::Diagnostics::StackTracesEnabled,
    cpptrace::runtime_error,
    std::runtime_error>;

template <typename... Args>
[[noreturn]] void throwError(std::format_string<const Args&...> fmt, const Args&... args) {
    throw RuntimeError(std::format(fmt, args...));
}

template <typename... Args>
void require(bool value, std::format_string<const Args&...> fmt, const Args&... args) {
    if (value) [[likely]] {
        return;
    }

    throwError(fmt, args...);
}

template <typename Error>
std::string_view message(const Error& err) {
    if constexpr (requires { err.message(); }) {
        return err.message();
    } else {
        return err.what();
    }
}

template <typename Error>
void printStackTrace(const Error& err, std::ostream& os) {
    if constexpr (requires { err.trace(); }) {
        auto formatter = cpptrace::get_default_formatter();
        formatter.header("Stack trace:")
            .addresses(cpptrace::formatter::address_mode::object)
            .break_before_filename()
            .colors(cpptrace::formatter::color_mode::automatic)
            .hide_exception_machinery()
            .symbols(cpptrace::formatter::symbol_mode::pruned)
            .snippets(true);

        formatter.print(os, err.trace());
    }
}

}  // namespace lsql

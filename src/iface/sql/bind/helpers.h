#pragma once

#include "iface/sql/ast/Literal.h"

#include "core/Value.h"

#include <format>

namespace lsql::iface::sql::bind {

template <typename... Args>
void require(bool value, std::format_string<const Args&...> fmt, const Args&... args) {
    if (value) [[likely]] {
        return;
    }

    throw std::runtime_error(std::format(fmt, args...));
}

inline std::string removeQuotes(const std::string& s) {
    require(s.size() >= 2, "string literal is too small");
    return s.substr(1, s.size() - 2);
}

inline Value parseLiteral(ast::Literal literal) {
    switch (literal.type) {
        case ValueType::Null:
            return null;

        case ValueType::String:
            return removeQuotes(literal.value_str);

        case ValueType::Integer:
            return int64_t(std::stoll(literal.value_str));

        case ValueType::Floating:
            return float(std::strtof(literal.value_str.data(), nullptr));

        case ValueType::Boolean:
            require(
                literal.value_str == "true" || literal.value_str == "false",
                "invalid boolean literal");
            return literal.value_str == "true";
    }
}

}  // namespace lsql::iface::sql::bind

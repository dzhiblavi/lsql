#pragma once

#include "front/common/source/SourceSpan.h"

#include "core/value/Value.h"
#include "core/value/ValueType.h"

#include "core/exceptions.h"

#include <magic_enum/magic_enum.hpp>

#include <string>

namespace lsql::front::common::ast {

struct Literal {
    ValueType type;
    std::string value_str;
    SourceSpan span;
};

inline std::string removeQuotes(const std::string& s) {
    require(s.size() >= 2, "string literal is too small");
    require(s.front() == '\'' && s.back() == '\'', "invalid string literal");
    return s.substr(1, s.size() - 2);
}

inline std::string to_string(const Literal& v) {
    return std::format("{}({})", magic_enum::enum_name(v.type), v.value_str);
}

inline std::optional<Value> parseLiteral(Literal literal) {
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
            if (literal.value_str != "true" && literal.value_str != "false") {
                return std::nullopt;
            }
            return literal.value_str == "true";
    }
}

}  // namespace lsql::front::common::ast

#pragma once

#include "core/Value.h"
#include "core/ValueType.h"

#include "util/require.h"

#include <magic_enum/magic_enum.hpp>

#include <string>

namespace lsql::front {

struct Literal {
    ValueType type;
    std::string value_str;
};

inline std::string removeQuotes(const std::string& s) {
    require(s.size() >= 2, "string literal is too small");
    return s.substr(1, s.size() - 2);
}

inline std::string to_string(const Literal& v) {
    return std::format("{}({})", magic_enum::enum_name(v.type), v.value_str);
}

inline Value parseLiteral(Literal literal) {
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

}  // namespace lsql::front

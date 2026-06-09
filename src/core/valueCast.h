#pragma once

#include "core/Value.h"
#include "util/string_cast.h"

namespace lsql {

inline std::optional<Value> valueCast(Value val, ValueType to) {
    static constexpr auto make_value = [](auto x) { return Value(std::move(x)); };

    return std::visit(
        util::Overloaded{
            [](null_t) -> std::optional<Value> { return null; },
            [&](std::string s) -> std::optional<Value> {
                switch (to) {
                    case ValueType::String:
                        return s;
                    case ValueType::Integer:
                        return util::parseInt64Strict(s).transform(make_value);
                    case ValueType::Boolean:
                        return !s.empty();
                    case ValueType::Floating:
                        return util::parseFloatStrict(s).transform(make_value);
                    case ValueType::Null:
                        return null;
                };
            },
            [&](int64_t x) -> std::optional<Value> {
                switch (to) {
                    case ValueType::String:
                        return std::to_string(x);
                    case ValueType::Integer:
                        return x;
                    case ValueType::Boolean:
                        return x != 0;
                    case ValueType::Floating:
                        return float(x);
                    case ValueType::Null:
                        return null;
                };
            },
            [&](float x) -> std::optional<Value> {
                switch (to) {
                    case ValueType::String:
                        return std::to_string(x);
                    case ValueType::Integer:
                        return int64_t(x);
                    case ValueType::Boolean:
                        return x != 0.f;
                    case ValueType::Floating:
                        return x;
                    case ValueType::Null:
                        return null;
                };
            },
            [&](bool x) -> std::optional<Value> {
                switch (to) {
                    case ValueType::String:
                        return x ? "true" : "false";
                    case ValueType::Integer:
                        return int64_t(x ? 1 : 0);
                    case ValueType::Boolean:
                        return x;
                    case ValueType::Floating:
                        return x ? 1.f : 0.f;
                    case ValueType::Null:
                        return null;
                };
            },
        },
        std::move(val).variant());
}

}  // namespace lsql

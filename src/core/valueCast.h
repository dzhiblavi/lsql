#pragma once

#include "core/Value.h"

namespace lsql {

inline Value valueCast(Value val, ValueType to) {
    return std::visit(
        util::Overloaded{
            [](null_t) -> Value { return null; },
            [&](const std::string& s) -> Value {
                switch (to) {
                    case ValueType::String:
                        return s;
                    case ValueType::Integer:
                        return int64_t(std::stoll(s));
                    case ValueType::Boolean:
                        return !s.empty();
                    case ValueType::Floating:
                        return float(std::strtof(s.data(), nullptr));
                    case ValueType::Null:
                        return null;
                };
            },
            [&](int64_t x) -> Value {
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
            [&](float x) -> Value {
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
            [&](bool x) -> Value {
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

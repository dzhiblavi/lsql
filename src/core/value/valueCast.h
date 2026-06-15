#pragma once

#include "core/value/Value.h"
#include "util/string_cast.h"

namespace lsql {

inline std::optional<Value> valueCast(Value val, ValueType to) {
    static constexpr auto make_value = [](auto x) { return Value(std::move(x)); };

    switch (val.type()) {
        case ValueType::Null:
            return vnull;

        case ValueType::String:
            switch (to) {
                case ValueType::String:
                    return val;
                case ValueType::Integer:
                    return util::parseInt64Strict(val.get<std::string_view>())
                        .transform(make_value);
                case ValueType::Boolean:
                    return !val.get<std::string_view>().empty();
                case ValueType::Floating:
                    return util::parseFloatStrict(val.get<std::string_view>())
                        .transform(make_value);
                case ValueType::Null:
                    return null;
            };

        case ValueType::Integer:
            switch (to) {
                case ValueType::String:
                    return std::to_string(val.get<int64_t>());
                case ValueType::Integer:
                    return val;
                case ValueType::Boolean:
                    return val.get<int64_t>() != 0;
                case ValueType::Floating:
                    return float(val.get<int64_t>());
                case ValueType::Null:
                    return vnull;
            };

        case ValueType::Floating:
            switch (to) {
                case ValueType::String:
                    return std::to_string(val.get<float>());
                case ValueType::Integer:
                    return int64_t(val.get<float>());
                case ValueType::Boolean:
                    return val.get<float>() != 0.f;
                case ValueType::Floating:
                    return val;
                case ValueType::Null:
                    return vnull;
            };

        case ValueType::Boolean:
            switch (to) {
                case ValueType::String:
                    return val.get<bool>() ? std::string("true") : std::string("false");
                case ValueType::Integer:
                    return int64_t(val.get<bool>() ? 1 : 0);
                case ValueType::Boolean:
                    return val;
                case ValueType::Floating:
                    return val.get<bool>() ? 1.f : 0.f;
                case ValueType::Null:
                    return vnull;
            };
    }
}

}  // namespace lsql

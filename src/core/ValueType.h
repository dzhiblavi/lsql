#pragma once

#include "core/null_t.h"
#include "util/overloaded.h"
#include "util/verify.h"

#include <string>
#include <variant>

namespace lsql {

enum class ValueType {
    Null,
    String,
    Integer,
    Floating,
    Boolean,
};

using ValueTypeVariant = std::variant<
    std::type_identity<null_t>,
    std::type_identity<int64_t>,
    std::type_identity<float>,
    std::type_identity<bool>,
    std::type_identity<std::string_view>>;

template <typename T>
constexpr ValueType valueType() {
    if constexpr (std::same_as<T, null_t>) {
        return ValueType::Null;
    } else if constexpr (std::same_as<T, int64_t>) {
        return ValueType::Integer;
    } else if constexpr (std::same_as<T, float>) {
        return ValueType::Floating;
    } else if constexpr (std::same_as<T, bool>) {
        return ValueType::Boolean;
    } else if constexpr (std::same_as<T, std::string>) {
        return ValueType::String;
    } else if constexpr (std::same_as<T, std::string_view>) {
        return ValueType::String;
    }

    panic("unsupported type");
}

inline ValueTypeVariant intoTypeVariant(ValueType type) {
    switch (type) {
        case ValueType::Null:
            return std::type_identity<null_t>{};
        case ValueType::String:
            return std::type_identity<std::string_view>{};
        case ValueType::Integer:
            return std::type_identity<int64_t>{};
        case ValueType::Floating:
            return std::type_identity<float>{};
        case ValueType::Boolean:
            return std::type_identity<bool>{};
    }
}

template <typename R, typename F, std::same_as<ValueType>... Vs>
R dispatch(F&& func, Vs... types) {
    auto f = util::Overloaded{
        std::forward<F>(func),
        [](...) -> R { panic("un-covered dispatch variant"); },
    };

    return std::apply(
        [&](auto... type_variant) { return std::visit(std::move(f), type_variant...); },
        std::tuple(intoTypeVariant(types)...));
}

}  // namespace lsql

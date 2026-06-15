#pragma once

#include "core/value/PinnedString.h"
#include "core/value/ValueType.h"
#include "core/value/null_t.h"
#include "util/overloaded.h"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

namespace lsql {

class Value {
 public:
    using VariantType = std::variant<null_t, int64_t, float, bool, std::string_view>;

    Value() = default;

    constexpr Value(null_t) {}
    Value(bool value);
    Value(int64_t value);
    Value(float value);
    Value(std::string value);
    Value(PinnedString value);

    ValueType type() const;

    bool operator==(const Value& rhs) const;
    std::partial_ordering operator<=>(const Value& rhs) const;

    size_t hash() const;
    VariantType variant() const;
    Value substr(size_t pos, size_t len) const;

    template <typename T>
    T get() const {
        if constexpr (std::same_as<T, bool>) {
            verify_dbg(std::holds_alternative<bool>(val_));
            return std::get<bool>(val_);
        } else if constexpr (std::same_as<T, int64_t>) {
            verify_dbg(std::holds_alternative<int64_t>(val_));
            return std::get<int64_t>(val_);
        } else if constexpr (std::same_as<T, float>) {
            verify_dbg(std::holds_alternative<float>(val_));
            return std::get<float>(val_);
        } else if constexpr (std::same_as<T, std::string_view>) {
            return std::visit(
                util::Overloaded{
                    [](const std::string& s) -> std::string_view { return s; },
                    [](const PinnedString& s) -> std::string_view { return s; },
                    [](auto&&...) -> std::string_view { panic("does not hold string"); },
                },
                val_);
        } else if constexpr (std::same_as<T, null_t>) {
            return null;
        } else {
            static_assert(false, "invalid Value type");
        }
    }

 private:
    using StorageVariant = std::variant<null_t, int64_t, float, bool, std::string, PinnedString>;
    StorageVariant val_;

    friend void swap(Value& a, Value& b) noexcept;
    friend std::string to_string(const Value& val);
    friend std::string to_string(Value&& val);
};

[[maybe_unused]] inline constexpr Value vnull = null;

std::string to_string(const Value& val);
std::string to_string(Value&& val);

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::Value> {
    size_t operator()(const lsql::Value& val) const noexcept;
};

template <>
struct hash<std::vector<lsql::Value>> {
    size_t operator()(const std::vector<lsql::Value>& vec) const noexcept;
};

}  // namespace std

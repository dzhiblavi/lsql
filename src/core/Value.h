#pragma once

#include "core/PinnedString.h"
#include "core/ValueType.h"
#include "core/null_t.h"
#include "util/overloaded.h"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

namespace lsql {

class Value {
 public:
    Value() = default;

    constexpr Value(null_t) {}
    Value(bool value) : val_(value) {}
    Value(int64_t value) : val_(value) {}
    Value(float value) : val_(value) {}
    Value(std::string value) : val_(std::move(value)) {}
    Value(PinnedString value) : val_(std::move(value)) {}

    ValueType type() const {
        return std::visit(
            util::Overloaded{
                []<typename T>(const T&) { return valueType<std::decay_t<T>>(); },
                [](const PinnedString&) { return ValueType::String; },
            },
            val_);
    }

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

    bool operator==(const Value& rhs) const {
        return std::visit(
            util::Overloaded{
                []<typename T>(const T& l, const T& r) { return l == r; },
                [](const PinnedString& a, const std::string& b) {
                    return std::string_view(a) == std::string_view(b);
                },
                [](const std::string& a, const PinnedString& b) {
                    return std::string_view(a) == std::string_view(b);
                },
                [](auto&&...) { return false; },
            },
            val_,
            rhs.val_);
    }

    std::partial_ordering operator<=>(const Value& rhs) const {
        return std::visit(
            util::Overloaded{
                []<typename T>(const T& l, const T& r) -> std::partial_ordering { return l <=> r; },
                [](const PinnedString& a, const std::string& b) -> std::partial_ordering {
                    return std::string_view(a) <=> std::string_view(b);
                },
                [](const std::string& a, const PinnedString& b) -> std::partial_ordering {
                    return std::string_view(a) <=> std::string_view(b);
                },
                [&](auto&&...) -> std::partial_ordering {
                    return val_.index() <=> rhs.val_.index();
                },
            },
            val_,
            rhs.val_);
    }

    size_t hash() const;

    friend void swap(Value& a, Value& b) noexcept { std::swap(a.val_, b.val_); }

    using VariantType = std::variant<null_t, int64_t, float, bool, std::string_view>;

    VariantType variant() const {
        return std::visit(
            util::Overloaded{
                [](const auto& val) -> VariantType { return val; },
                [](const std::string& s) -> VariantType { return std::string_view(s); },
                [](const PinnedString& s) -> VariantType { return std::string_view(s); },
            },
            val_);
    }

    Value substr(size_t pos, size_t len) const {
        return std::visit(
            util::Overloaded{
                [&](const std::string& s) -> Value { return s.substr(pos, len); },
                [&](const PinnedString& s) -> Value { return s.substr(pos, len); },
                [](auto&&...) -> Value { panic(); },
            },
            val_);
    }

 private:
    using StorageVariant = std::variant<null_t, int64_t, float, bool, std::string, PinnedString>;

    StorageVariant val_;

    friend std::string to_string(const Value& val);
    friend std::string to_string(Value&& val);
};

inline std::string to_string(const Value& val) {
    return std::visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](const std::string& x) -> std::string { return x; },
            [](const PinnedString& x) -> std::string { return to_string(x); },
        },
        val.val_);
}

inline std::string to_string(Value&& val) {
    return std::visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](std::string x) -> std::string { return x; },
            [](PinnedString x) -> std::string { return to_string(x); },
        },
        std::move(val).val_);
}

[[maybe_unused]] inline constexpr Value vnull = null;

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::null_t> {
    size_t operator()(const lsql::null_t&) const noexcept { return 0x9e3779b9; }
};

}  // namespace std

namespace lsql {

inline size_t Value::hash() const {
    auto val_hash = std::visit(
        util::Overloaded{
            []<typename T>(const T& val) { return std::hash<T>{}(val); },
            [](const std::string& val) { return std::hash<std::string_view>{}(val); },
            [](const PinnedString& val) { return std::hash<std::string_view>{}(val); },
        },
        val_);

    auto index = std::min(val_.index(), size_t(4));  // 4 = index of std::string, TODO fragile
    val_hash ^= std::hash<size_t>{}(index) + 0x9e3779b9 + (val_hash << 6) + (val_hash >> 2);
    return val_hash;
}

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::Value> {
    size_t operator()(const lsql::Value& val) const noexcept { return val.hash(); }
};

template <>
struct hash<std::vector<lsql::Value>> {
    size_t operator()(const std::vector<lsql::Value>& vec) const noexcept {
        size_t seed = vec.size();
        for (const auto& val : vec) {
            seed ^= hash<lsql::Value>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

}  // namespace std

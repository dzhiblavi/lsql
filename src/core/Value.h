#pragma once

#include "core/ValueType.h"
#include "util/overloaded.h"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

namespace lsql {

struct null_t {
    auto operator<=>(const null_t&) const = default;
};

[[maybe_unused]] static constexpr null_t null{};

class Value {
 public:
    Value() = default;

    Value(null_t) {}
    Value(bool value) : val_(value) {}
    Value(int64_t value) : val_(value) {}
    Value(float value) : val_(value) {}
    Value(std::string value) : val_(std::move(value)) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T>(val_);
    }

    template <typename T>
    const T& get() const {
        if (const T* val = std::get_if<T>(&val_)) {
            return *val;
        }
        throw std::runtime_error("type mismatch");
    }

    ValueType type() const {
        return std::visit(
            util::Overloaded{
                [](null_t) -> ValueType { return ValueType::Null; },
                [](bool) -> ValueType { return ValueType::Boolean; },
                [](int64_t) -> ValueType { return ValueType::Integer; },
                [](float) -> ValueType { return ValueType::Floating; },
                [](const std::string&) -> ValueType { return ValueType::String; },
            },
            val_);
    }

    auto operator<=>(const Value& rhs) const = default;

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        return std::visit(std::forward<Visitor>(vis), val_);
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        return std::visit(std::forward<Visitor>(vis), val_);
    }

    size_t hash() const;

    template <typename Visitor, typename... Values>
    friend decltype(auto) visit(Visitor&& vis, Values&&... values) {
        return std::visit(std::forward<Visitor>(vis), std::forward<Values>(values).val_...);
    }

    friend void swap(Value& a, Value& b) noexcept { std::swap(a.val_, b.val_); }

 private:
    std::variant<null_t, int64_t, float, std::string, bool> val_;
};

inline std::string to_string(const Value& val) {
    return val.visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](const std::string& x) -> std::string { return x; },
        });
}

inline bool trueish(const Value& val) {
    return val.visit(
        util::Overloaded{
            [](const std::string& s) { return !s.empty(); },
            [](bool b) { return b; },
            [](int64_t x) { return x != 0; },
            [](float x) { return abs(x) > 1e-6f; },
            [](null_t) { return false; },
        });
}

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::null_t> {
    size_t operator()(const lsql::null_t&) const noexcept { return 0x9e3779b9; }
};

}  // namespace std

namespace lsql {

inline size_t Value::hash() const {
    return std::hash<decltype(val_)>{}(val_);
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

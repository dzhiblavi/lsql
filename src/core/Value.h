#pragma once

#include "core/ValueType.h"
#include "core/null_t.h"
#include "core/require.h"
#include "util/overloaded.h"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

namespace lsql {

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
        const T* val = std::get_if<T>(&val_);
        require(val != nullptr, "type mismatch");
        return *val;
    }

    ValueType type() const {
        return std::visit([]<typename T>(T&&) { return valueType<std::decay_t<T>>(); }, val_);
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

    auto& variant() const { return val_; }
    auto& variant() { return val_; }

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

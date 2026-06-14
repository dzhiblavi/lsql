#include "core/Value.h"

namespace lsql {

Value::Value(bool value) : val_(value) {
}

Value::Value(int64_t value) : val_(value) {
}

Value::Value(float value) : val_(value) {
}

Value::Value(std::string value) : val_(std::move(value)) {
}

Value::Value(PinnedString value) : val_(std::move(value)) {
}

ValueType Value::type() const {
    return std::visit(
        util::Overloaded{
            []<typename T>(const T&) { return valueType<std::decay_t<T>>(); },
            [](const PinnedString&) { return ValueType::String; },
        },
        val_);
}

bool Value::operator==(const Value& rhs) const {
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

std::partial_ordering Value::operator<=>(const Value& rhs) const {
    return std::visit(
        util::Overloaded{
            []<typename T>(const T& l, const T& r) -> std::partial_ordering { return l <=> r; },
            [](const PinnedString& a, const std::string& b) -> std::partial_ordering {
                return std::string_view(a) <=> std::string_view(b);
            },
            [](const std::string& a, const PinnedString& b) -> std::partial_ordering {
                return std::string_view(a) <=> std::string_view(b);
            },
            [&](auto&&...) -> std::partial_ordering { return val_.index() <=> rhs.val_.index(); },
        },
        val_,
        rhs.val_);
}

size_t Value::hash() const {
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

Value::VariantType Value::variant() const {
    return std::visit(
        util::Overloaded{
            [](const auto& val) -> VariantType { return val; },
            [](const std::string& s) -> VariantType { return std::string_view(s); },
            [](const PinnedString& s) -> VariantType { return std::string_view(s); },
        },
        val_);
}

Value Value::substr(size_t pos, size_t len) const {
    return std::visit(
        util::Overloaded{
            [&](const std::string& s) -> Value { return s.substr(pos, len); },
            [&](const PinnedString& s) -> Value { return s.substr(pos, len); },
            [](auto&&...) -> Value { panic(); },
        },
        val_);
}

void swap(Value& a, Value& b) noexcept {
    std::swap(a.val_, b.val_);
}

std::string to_string(const Value& val) {
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

std::string to_string(Value&& val) {
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

}  // namespace lsql

size_t std::hash<lsql::Value>::operator()(const lsql::Value& val) const noexcept {
    return val.hash();
}

size_t std::hash<std::vector<lsql::Value>>::operator()(
    const std::vector<lsql::Value>& vec) const noexcept {
    size_t seed = vec.size();
    for (const auto& val : vec) {
        seed ^= hash<lsql::Value>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

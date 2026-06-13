#pragma once

#include "core/types.h"

#include <string>

namespace lsql {

class PinnedString {
 public:
    PinnedString() = default;
    PinnedString(Arc<const char> pin, size_t size) : pin_(std::move(pin)), size_(size) {}

    std::string_view view() const { return {pin_.get(), size_}; }
    operator std::string_view() const { return view(); }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    PinnedString substr(size_t pos, size_t len) const {
        auto v = view().substr(pos, len);
        return {Arc<const char>(pin_, v.data()), v.size()};
    }

 public:
    Arc<const char> pin_ = nullptr;
    size_t size_ = 0;
};

inline bool operator==(const PinnedString& a, const PinnedString& b) {
    return a.view() == b.view();
}

inline std::strong_ordering operator<=>(const PinnedString& a, const PinnedString& b) {
    return a.view() <=> b.view();
}

inline std::string to_string(const PinnedString& p) {
    return std::string(p.view());
}

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::PinnedString> {
    size_t operator()(const lsql::PinnedString& val) const noexcept {
        return std::hash<std::string_view>()(val.view());
    }
};

}  // namespace std

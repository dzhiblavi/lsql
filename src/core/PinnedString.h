#pragma once

#include "core/types.h"

#include <string>

namespace lsql {

class PinnedString {
 public:
    PinnedString() = default;
    PinnedString(Arc<const void> pin, std::string_view view) : pin_(std::move(pin)), view_(view) {}

    std::string_view view() const { return view_; }
    operator std::string_view() const { return view_; }
    size_t size() const { return view_.size(); }
    bool empty() const { return view_.empty(); }

    PinnedString substr(size_t pos, size_t len) const {
        return {
            pin_,
            view_.substr(pos, len),
        };
    }

 public:
    Arc<const void> pin_ = nullptr;
    std::string_view view_;
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

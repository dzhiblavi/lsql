#pragma once

#include "core/types.h"

#include <string>

namespace lsql {

class PinnedString {
 public:
    PinnedString() = default;
    PinnedString(Arc<const char> pin, size_t size);

    std::string_view view() const;
    operator std::string_view() const;
    size_t size() const;
    bool empty() const;
    PinnedString substr(size_t pos, size_t len) const;
    PinnedString subview(std::string_view view) const;

 public:
    Arc<const char> pin_ = nullptr;
    size_t size_ = 0;
};

bool operator==(const PinnedString& a, const PinnedString& b);
std::strong_ordering operator<=>(const PinnedString& a, const PinnedString& b);
std::string to_string(const PinnedString& p);

}  // namespace lsql

template <>
struct std::hash<lsql::PinnedString> {
    size_t operator()(const lsql::PinnedString& val) const noexcept;
};

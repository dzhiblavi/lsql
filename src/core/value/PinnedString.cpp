#include "core/value/PinnedString.h"

namespace lsql {

PinnedString::PinnedString(Arc<const char> pin, size_t size) : pin_(std::move(pin)), size_(size) {
}

std::string_view PinnedString::view() const {
    return {pin_.get(), size_};
}

PinnedString::operator std::string_view() const {
    return view();
}

size_t PinnedString::size() const {
    return size_;
}

bool PinnedString::empty() const {
    return size_ == 0;
}

PinnedString PinnedString::substr(size_t pos, size_t len) const {
    return subview(view().substr(pos, len));
}

PinnedString PinnedString::subview(std::string_view view) const {
    return {Arc<const char>(pin_, view.data()), view.size()};
}

bool operator==(const PinnedString& a, const PinnedString& b) {
    return a.view() == b.view();
}

std::strong_ordering operator<=>(const PinnedString& a, const PinnedString& b) {
    return a.view() <=> b.view();
}

std::string to_string(const PinnedString& p) {
    return std::string(p.view());
}

}  // namespace lsql

size_t std::hash<lsql::PinnedString>::operator()(const lsql::PinnedString& val) const noexcept {
    return std::hash<std::string_view>()(val.view());
}

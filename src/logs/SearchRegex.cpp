#include "logs/SearchRegex.h"
#include "util/NonCopyable.h"

#include <algorithm>
#include <reflex/matcher.h>

namespace lsql::logs {

namespace {

class reverse_streambuf : public std::streambuf, util::NonCopyable {
 public:
    reverse_streambuf(const char* data, size_t size)
        : begin_(data)
        , current_(data + size) {}  // NOLINT

 private:
    int underflow() override {
        size_t bytes_remaining = current_ - begin_;
        if (bytes_remaining == 0) {
            return std::char_traits<char>::eof();
        }

        size_t bytes_to_read = std::min(BufSize, bytes_remaining);
        current_ -= bytes_to_read;                                            // NOLINT
        std::reverse_copy(current_, current_ + bytes_to_read, buf_.begin());  // NOLINT

        setg(buf_.begin(), buf_.begin(), buf_.begin() + bytes_to_read);
        return std::char_traits<char>::to_int_type(*gptr());
    }

    const char* begin_;
    const char* current_;

    static constexpr size_t BufSize = 2048;
    std::array<char, BufSize> buf_{};
};

}  // namespace

std::optional<std::string_view> findFirst(std::string_view s, const reflex::Pattern& pattern) {
    reflex::Matcher matcher(&pattern, reflex::Input(s.data(), s.size()));

    if (!matcher.find()) {
        return std::nullopt;
    }

    return s.substr(matcher.first(), matcher.size());
}

std::optional<std::string_view> findLast(std::string_view s, const reflex::Pattern& rev_pattern) {
    reverse_streambuf buf(s.data(), s.size());
    std::istream ss(&buf);

    reflex::Matcher matcher(&rev_pattern, reflex::Input(&ss));

    if (!matcher.find()) {
        return std::nullopt;
    }

    size_t forward_first = s.size() - matcher.first() - matcher.size();
    return s.substr(forward_first, matcher.size());
}

std::optional<timestamp_t> searchFirstTimestamp(std::string_view s, TimeFormat format) {
    return findFirst(s, timeFormatPattern(format)).transform([&](std::string_view sv) {
        return timestampFromString(sv, format);
    });
}

std::optional<timestamp_t> searchLastTimestamp(std::string_view s, TimeFormat format) {
    return findLast(s, timeFormatReversePattern(format)).transform([&](std::string_view sv) {
        return timestampFromString(sv, format);
    });
}

}  // namespace lsql::logs

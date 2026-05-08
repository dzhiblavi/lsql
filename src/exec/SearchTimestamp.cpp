#include "exec/SearchTimestamp.h"

#include "exec/SearchRegex.h"
#include "util/PageSize.h"

#include <cassert>

namespace lsql::exec {

namespace {

template <typename GetPageF, typename FirstTsF, typename LastTsF>
size_t lowerBoundPageImpl(
    const data::PagedFile& file, timestamp_t ts, GetPageF get_page, FirstTsF first, LastTsF last) {
    size_t begin = 0;
    size_t end = file.pageCount();

    // searching in [begin, end)
    while (begin + 1 < end) {
        size_t mid = begin + (end - begin) / 2;
        timestamp_t lower_ts = 0;

        for (;;) {
            auto page = get_page(mid);
            auto maybe_lower_ts = first(page->data());
            if (maybe_lower_ts) {
                lower_ts = *maybe_lower_ts;
                break;
            }

            if (++mid >= file.pageCount()) {
                throw std::runtime_error("fix me if I fire (1) {}");
            }
        }

        if (lower_ts < ts) {
            begin = mid;
            continue;
        }

        // lower_ts >= ts
        assert(mid > 0);
        timestamp_t prev_upper_ts = 0;

        for (;;) {
            auto prev_page = get_page(mid - 1);
            auto maybe_prev_upper_ts = last(prev_page->data());

            if (maybe_prev_upper_ts) {
                prev_upper_ts = *maybe_prev_upper_ts;
                break;
            }

            if (--mid == 0) {
                throw std::runtime_error("fix me if I fire (2)");
            }
        }

        if (prev_upper_ts >= ts) {
            // previous page also has the needed range
            end = mid;
            continue;
        }

        // lower_ts <= ts
        // prev_upper_ts < ts
        // that means that we've found the needed page, it is mid.
        return mid;
    }

    auto page = get_page(begin);
    auto maybe_upper_ts = last(page->data());
    if (!maybe_upper_ts) {
        throw std::runtime_error("fix me pls");
    }

    auto upper_ts = *maybe_upper_ts;
    return upper_ts >= ts ? begin : std::string::npos;
}

size_t getSincePos(std::string_view s, timestamp_t from, TimeFormat format) {
    auto pos = s.find('\n');
    if (pos == std::string::npos) {
        return std::string::npos;
    }
    ++pos;  // points to the first character of the line

    while (pos < s.size()) {
        auto line_length = s.substr(pos).find('\n');
        auto line_ts = searchFirstTimestamp(s.substr(pos, line_length), format);
        if (line_ts >= from) {
            return pos;
        }

        if (line_length == std::string::npos) {
            return std::string::npos;
        }

        pos += line_length + 1;
    }

    return std::string::npos;
}

size_t getUntilPos(std::string_view s, timestamp_t to, TimeFormat format) {
    auto pos = s.find('\n');
    if (pos == std::string::npos) {
        return std::string::npos;
    }
    ++pos;  // points to the first character of the line

    while (pos < s.size()) {
        auto line_length = s.substr(pos).find('\n');
        auto line_ts = searchFirstTimestamp(s.substr(pos, line_length), format);
        if (line_ts > to) {
            return pos;
        }

        if (line_length == std::string::npos) {
            return s.size();
        }

        pos += line_length + 1;
    }

    return s.size();
}

}  // namespace

size_t lowerBoundPage(const data::PagedFile& file, timestamp_t ts, TimeFormat format) {
    return lowerBoundPageImpl(
        file,
        ts,
        [&file](size_t page_index) { return file.page(page_index); },
        [&](auto data) { return searchFirstTimestamp(data, format); },
        [&](auto data) { return searchLastTimestamp(data, format); });
}

size_t upperBoundPage(const data::PagedFile& file, timestamp_t ts, TimeFormat format) {
    auto pc = file.pageCount();

    auto res = lowerBoundPageImpl(
        file,
        -ts,
        [&file, pc](size_t page_index) { return file.page(pc - page_index - 1); },
        [&](auto data) { return searchLastTimestamp(data, format).transform(std::negate{}); },
        [&](auto data) { return searchFirstTimestamp(data, format).transform(std::negate{}); });

    return res == std::string::npos ? std::string::npos : pc - 1 - res;
}

size_t lowerBoundLine(const data::PagedFile& file, timestamp_t ts, TimeFormat format) {
    size_t p = lowerBoundPage(file, ts, format);
    if (p == std::string::npos) {
        return std::string::npos;
    }
    size_t pos = getSincePos(file.page(p)->data(), ts, format);
    return pos == std::string::npos ? pos : pos + p * util::pageSize();
}

size_t upperBoundLine(const data::PagedFile& file, timestamp_t ts, TimeFormat format) {
    size_t p = upperBoundPage(file, ts, format);
    if (p == std::string::npos) {
        return std::string::npos;
    }
    size_t pos = getUntilPos(file.page(p)->data(), ts, format);
    return pos == std::string::npos ? pos : pos + p * util::pageSize();
}

}  // namespace lsql::exec

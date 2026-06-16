#include "back/plan/search/SearchTimestamp.h"
#include "back/plan/search/SearchRegex.h"

#include "config/build_settings.h"
#include "core/exceptions.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>

namespace lsql::back::plan::search {

namespace {

struct Line {
    size_t begin;
    size_t next_begin;
    std::string text;
};

std::optional<char> readChar(const back::storage::File& file, size_t offset) {
    if (offset >= file.size()) {
        return std::nullopt;
    }

    char ch = 0;
    std::ignore = file.read(offset, std::span{&ch, 1});
    return ch;
}

std::optional<size_t> findLineBegin(const back::storage::File& file, size_t offset) {
    if (offset >= file.size()) {
        return std::nullopt;
    }

    if (offset == 0) {
        return 0;
    }

    auto prev = readChar(file, offset - 1);
    if (prev == '\n') {
        return offset;
    }

    std::array<char, config::Buffering::TimestampSearchBufferSize> buffer{};
    while (offset > 0) {
        size_t chunk_begin = offset > buffer.size() ? offset - buffer.size() : 0;
        size_t chunk_size = offset - chunk_begin;
        std::ignore = file.read(chunk_begin, std::span{buffer.data(), chunk_size});

        auto begin = buffer.begin();
        auto end = begin + chunk_size;  // NOLINT
        auto newline = std::find(std::reverse_iterator{end}, std::reverse_iterator{begin}, '\n');
        if (newline != std::reverse_iterator{begin}) {
            return chunk_begin + static_cast<size_t>((newline.base() - begin));
        }

        offset = chunk_begin;
    }

    return 0;
}

Line readLine(const back::storage::File& file, size_t begin) {
    std::array<char, config::Buffering::TimestampSearchBufferSize> buffer{};
    std::string text;
    size_t offset = begin;

    while (offset < file.size()) {
        size_t bytes_read = file.read(
            offset, std::span{buffer.data(), std::min(buffer.size(), file.size() - offset)});
        auto chunk_begin = buffer.begin();
        auto chunk_end = chunk_begin + bytes_read;  // NOLINT
        auto newline = std::find(chunk_begin, chunk_end, '\n');

        text.append(chunk_begin, newline);

        if (newline != chunk_end) {
            auto next_begin = offset + static_cast<size_t>(newline - chunk_begin) + 1;
            return Line{
                .begin = begin,
                .next_begin = next_begin,
                .text = std::move(text),
            };
        }

        offset += bytes_read;
    }

    return Line{.begin = begin, .next_begin = file.size(), .text = std::move(text)};
}

std::optional<Line> readLineAt(const back::storage::File& file, size_t offset) {
    auto begin = findLineBegin(file, offset);
    if (!begin) {
        return std::nullopt;
    }

    return readLine(file, *begin);
}

timestamp_t lineTimestamp(std::string_view line, TimeFormat format) {
    auto maybe_ts = searchFirstTimestamp(line, format);
    require(maybe_ts.has_value(), "failed to find timestamp in line '{}'", line);
    return *maybe_ts;
}

template <typename AcceptLineF, typename SkipLineF>
size_t boundLine(
    const back::storage::File& file,
    timestamp_t ts,
    TimeFormat format,
    AcceptLineF accept_line,
    SkipLineF skip_line) {
    size_t begin = 0;
    size_t end = file.size();
    size_t answer = std::string::npos;

    while (begin < end) {
        size_t mid = begin + (end - begin) / 2;
        auto line = readLineAt(file, mid);

        if (!line) {
            end = mid;
            continue;
        }

        auto line_ts = lineTimestamp(line->text, format);
        if (skip_line(line_ts, ts)) {
            begin = line->next_begin;
            continue;
        }

        if (accept_line(line_ts, ts)) {
            answer = line->begin;
        }
        end = line->begin;
    }

    return answer;
}

}  // namespace

size_t lowerBoundLine(const back::storage::File& file, timestamp_t ts, TimeFormat format) {
    return boundLine(
        file,
        ts,
        format,
        [](timestamp_t line_ts, timestamp_t target) { return line_ts >= target; },
        [](timestamp_t line_ts, timestamp_t target) { return line_ts < target; });
}

size_t upperBoundLine(const back::storage::File& file, timestamp_t ts, TimeFormat format) {
    auto result = boundLine(
        file,
        ts,
        format,
        [](timestamp_t line_ts, timestamp_t target) { return line_ts > target; },
        [](timestamp_t line_ts, timestamp_t target) { return line_ts <= target; });
    return result == std::string::npos ? file.size() : result;
}

}  // namespace lsql::back::plan::search

#include "back/exec/phys/log/log.h"
#include "back/exec/phys/log/search/SearchTimestamp.h"

#include "back/storage/Archive.h"
#include "back/storage/CommandStream.h"
#include "back/storage/LineSource.h"
#include "back/storage/PagedFile.h"

#include "back/logfmt/LogType.h"
#include "back/logfmt/log_types.h"

#include "core/exceptions.h"
#include "util/archive.h"

#include <cpptrace/exceptions.hpp>

namespace lsql::back::exec::phys {

namespace {

using namespace exec;

std::optional<back::logfmt::LogType> detectLogType(const back::storage::LineSource& source) {
    for (auto&& line : source.lines()) {
        if (auto type = back::logfmt::detectLogType(line.view())) {
            return type;
        }
    }

    return std::nullopt;
}

LogFile getFileSourceWhole(std::string path) {
    auto file = back::storage::NativePagedFile::open(path);
    Arc<back::storage::LineSource> line_source;

    if (util::isProbablyArchive(path)) {
        auto stream_source = arc<back::storage::NativeArchive>(file);
        line_source = arc<back::storage::StreamLineSource>(stream_source);
    } else {
        line_source = arc<back::storage::PagedLineSource>(file);
    }

    return {
        .lines = line_source,
        .type = detectLogType(*line_source),
    };
}

LogFile getFileSourceRange(
    std::string path, TimeRange range, std::optional<TimeFormat> time_format) {
    require(
        !util::isProbablyArchive(path),
        "time range cannot be applied to compressed streams, path: '{}'",
        path);

    auto file = back::storage::NativePagedFile::open(path);
    auto whole_line_source = arc<back::storage::PagedLineSource>(file);
    auto log_type = detectLogType(*whole_line_source);
    if (!time_format.has_value()) {
        require(log_type.has_value(), "failed to detect log type");
        time_format = back::logfmt::timeFormat(*log_type);
    }

    require(time_format.has_value(), "can't pull timestamp from file at {}", path);

    llog::trace("searching for {} in file {}", range.ts_from, file->path().c_str());
    auto from_pos = lowerBoundLine(*file, range.ts_from, *time_format);

    llog::trace("searching for {} in file {}", range.ts_to, file->path().c_str());
    auto to_pos = upperBoundLine(*file, range.ts_to, *time_format);

    if (from_pos == std::string::npos || to_pos == std::string::npos || to_pos <= from_pos) {
        from_pos = to_pos = 0;
    }

    auto line_source = arc<back::storage::PagedLineSource>(file, from_pos, to_pos);
    return {
        .lines = line_source,
        .type = log_type,
    };
}

LogFile getFileSource(
    std::string path, std::optional<TimeRange> range, std::optional<TimeFormat> time_format) {
    if (range.has_value()) {
        return getFileSourceRange(path, *range, time_format);
    }
    return getFileSourceWhole(path);
}

}  // namespace

LogFile open(const plan::Log& log) {
    return getFileSource(log.path, log.range, log.time_format);
}

LogFile open(const plan::Stream& stream) {
    auto stream_source = arc<back::storage::CommandStreamSource>(stream.command);
    auto line_source = arc<back::storage::StreamLineSource>(stream_source);
    return {
        .lines = line_source,
        .type = std::nullopt,
    };
}

}  // namespace lsql::back::exec::phys

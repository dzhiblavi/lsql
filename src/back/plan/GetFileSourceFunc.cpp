#include "back/plan/GetFileSourceFunc.h"

#include "back/storage/Archive.h"
#include "back/storage/LineSource.h"
#include "back/storage/PagedFile.h"

#include "back/logfmt/LogType.h"
#include "back/logfmt/log_types.h"
#include "back/plan/search/SearchTimestamp.h"

#include "back/exec/op/Log.h"

#include "core/exceptions.h"
#include "util/archive.h"

#include <cpptrace/exceptions.hpp>

namespace lsql::back::plan {

namespace {

using namespace exec;

back::logfmt::LogType getLogType(const back::storage::LineSource& source) {
    for (auto&& line : source.lines()) {
        if (auto type = back::logfmt::detectLogType(line.view())) {
            return *type;
        }

        break;
    }

    throwError("failed to detect log type");
}

SourcePtr getFileSourceWhole(std::string path, Schema schema, ConstFieldBindingPtr binding) {
    auto file = back::storage::NativePagedFile::open(path);
    Arc<back::storage::LineSource> line_source;

    if (util::isProbablyArchive(path)) {
        auto stream_source = arc<back::storage::NativeArchive>(file);
        line_source = arc<back::storage::StreamLineSource>(stream_source);
    } else {
        line_source = arc<back::storage::PagedLineSource>(file);
    }

    return arc<Log>(line_source, getLogType(*line_source), schema, std::move(binding));
}

SourcePtr getFileSourceRange(
    std::string path, Schema schema, TimeRange range, ConstFieldBindingPtr binding) {
    require(
        !util::isProbablyArchive(path),
        "time range cannot be applied to compressed streams, path: '{}'",
        path);

    auto file = back::storage::NativePagedFile::open(path);
    auto whole_line_source = arc<back::storage::PagedLineSource>(file);
    auto log_type = getLogType(*whole_line_source);
    auto time_format = back::logfmt::timeFormat(log_type);

    llog::trace("searching for {} in file {}", range.ts_from, file->path().c_str());
    auto from_pos = search::lowerBoundLine(*file, range.ts_from, time_format);

    llog::trace("searching for {} in file {}", range.ts_to, file->path().c_str());
    auto to_pos = search::upperBoundLine(*file, range.ts_to, time_format);

    if (from_pos == std::string::npos || to_pos == std::string::npos || to_pos <= from_pos) {
        from_pos = to_pos = 0;
    }

    auto line_source = arc<back::storage::PagedLineSource>(file, from_pos, to_pos);
    return arc<Log>(line_source, log_type, schema, std::move(binding));
}

SourcePtr getFileSource(
    std::string path, Schema schema, ConstFieldBindingPtr binding, std::optional<TimeRange> range) {
    if (range.has_value()) {
        return getFileSourceRange(path, schema, *range, std::move(binding));
    }
    return getFileSourceWhole(path, schema, std::move(binding));
}

}  // namespace

GetFileSourceFuncType defaultFileSourceFunc() {
    return &getFileSource;
}

}  // namespace lsql::back::plan

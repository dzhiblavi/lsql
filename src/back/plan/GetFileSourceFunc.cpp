#include "back/plan/GetFileSourceFunc.h"

#include "back/storage/LineSource.h"
#include "back/storage/PagedFile.h"

#include "back/logfmt/LogType.h"
#include "back/logfmt/log_types.h"
#include "back/plan/search/SearchTimestamp.h"

#include "back/exec/op/Log.h"

namespace lsql::back::plan {

namespace {

using namespace exec;

back::logfmt::LogType getLogType(const back::storage::PagedFile& file) {
    // TODO: this assumes the file starts with a line
    if (auto type = back::logfmt::detectLogType(file.page(0)->data())) {
        return *type;
    }

    throw std::runtime_error("failed to detect log type");
}

SourcePtr getFileSourceWhole(std::string path, ConstFieldBindingPtr binding) {
    auto file = back::storage::NativePagedFile::open(path);
    return std::make_shared<Log>(
        std::make_shared<back::storage::PagedLineSource>(file),
        getLogType(*file),
        std::move(binding));
}

SourcePtr getFileSourceRange(std::string path, TimeRange range, ConstFieldBindingPtr binding) {
    auto file = back::storage::NativePagedFile::open(path);
    auto log_type = getLogType(*file);
    auto time_format = back::logfmt::timeFormat(log_type);
    auto from_pos = search::lowerBoundLine(*file, range.ts_from, time_format);
    auto to_pos = search::upperBoundLine(*file, range.ts_to, time_format);

    if (from_pos == std::string::npos || to_pos <= from_pos) {
        from_pos = to_pos = 0;
    }

    return std::make_shared<Log>(
        std::make_shared<back::storage::PagedLineSource>(file, from_pos, to_pos),
        log_type,
        std::move(binding));
}

SourcePtr getFileSource(
    std::string path, ConstFieldBindingPtr binding, std::optional<TimeRange> range) {
    if (range.has_value()) {
        return getFileSourceRange(path, *range, std::move(binding));
    }
    return getFileSourceWhole(path, std::move(binding));
}

}  // namespace

GetFileSourceFuncType defaultFileSourceFunc() {
    return &getFileSource;
}

}  // namespace lsql::back::plan

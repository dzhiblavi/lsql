#include "exec/plan/GetFileSourceFunc.h"

#include "data/Log.h"
#include "data/PagedFile.h"

#include "logs/LogType.h"
#include "logs/SearchTimestamp.h"
#include "logs/log_types.h"

#include "exec/op/Log.h"

namespace lsql::exec {

namespace {

logs::LogType getLogType(const data::PagedFile& file) {
    // TODO: this assumes the file starts with a line
    if (auto type = logs::detectLogType(file.page(0)->data())) {
        return *type;
    }

    throw std::runtime_error("failed to detect log type");
}

SourcePtr getFileSourceWhole(std::string path, ConstFieldBindingPtr binding) {
    auto file = data::NativePagedFile::open(path);
    return std::make_shared<Log>(
        std::make_shared<data::PagedLog>(file), getLogType(*file), std::move(binding));
}

SourcePtr getFileSourceRange(std::string path, TimeRange range, ConstFieldBindingPtr binding) {
    auto file = data::NativePagedFile::open(path);
    auto log_type = getLogType(*file);
    auto time_format = logs::timeFormat(log_type);
    auto from_pos = logs::lowerBoundLine(*file, range.ts_from, time_format);
    auto to_pos = logs::upperBoundLine(*file, range.ts_to, time_format);

    if (from_pos == std::string::npos || to_pos <= from_pos) {
        from_pos = to_pos = 0;
    }

    return std::make_shared<Log>(
        std::make_shared<data::PagedLog>(file, from_pos, to_pos), log_type, std::move(binding));
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

}  // namespace lsql::exec

#include "interface/sql/Interface.h"

#include "interface/sql/ExecVisitor.h"
#include "interface/sql/parser/parse.h"

#include "data/Log.h"
#include "data/PagedFile.h"

#include "logs/LogType.h"
#include "logs/SearchTimestamp.h"
#include "logs/log_types.h"

#include "exec/op/Log.h"

namespace lsql::iface::sql {

namespace {

logs::LogType getLogType(const data::PagedFile& file) {
    // TODO: this assumes the file starts with a line
    if (auto type = logs::detectLogType(file.page(0)->data())) {
        return *type;
    }

    throw std::runtime_error("failed to detect log type");
}

exec::SourcePtr getFileSourceWhole(std::string path) {
    auto file = data::NativePagedFile::open(path);
    return std::make_shared<exec::Log>(std::make_shared<data::PagedLog>(file), getLogType(*file));
}

exec::SourcePtr getFileSourceRange(std::string path, TimeRange range) {
    auto from = timestampFromString(range.ts_from, TimeFormat::ISO8601);
    auto to = from + range.interval_s;

    auto file = data::NativePagedFile::open(path);
    auto log_type = getLogType(*file);
    auto time_format = logs::timeFormat(log_type);
    auto from_pos = logs::lowerBoundLine(*file, from, time_format);
    auto to_pos = logs::upperBoundLine(*file, to, time_format);

    if (from_pos == std::string::npos || to_pos <= from_pos) {
        from_pos = to_pos = 0;
    }

    return std::make_shared<exec::Log>(
        std::make_shared<data::PagedLog>(file, from_pos, to_pos), log_type);
}

exec::SourcePtr getFileSource(std::string path, std::optional<TimeRange> range) {
    if (range.has_value()) {
        return getFileSourceRange(path, *range);
    }
    return getFileSourceWhole(path);
}

}  // namespace

GetFileSourceFuncType defaultFileSourceFunc() {
    return &getFileSource;
}

Plan Interface::plan() const {
    auto root = parse::parse(*query_in_);

    ExecVisitor exec_visitor(get_file_source_func_);
    root->visit(exec_visitor);
    auto [sources, operations] = std::move(exec_visitor).result();

    Plan plan;
    plan.sources = std::move(sources);
    plan.top_operations.reserve(operations.size());

    while (!operations.empty()) {
        plan.top_operations.push_back(std::move(operations.top()));
        operations.pop();
    }

    std::ranges::reverse(plan.top_operations);
    return plan;
}

}  // namespace lsql::iface::sql

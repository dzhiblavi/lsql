#pragma once

#include "back/exec/plan/Operation.h"
#include "back/logfmt/LogType.h"
#include "back/storage/LineSource.h"

namespace lsql::back::exec::phys {

struct LogFile {
    Arc<back::storage::LineSource> lines;
    std::optional<logfmt::LogType> type;
};

LogFile open(const plan::Log& log);
LogFile open(const plan::Stream& stream);

}  // namespace lsql::back::exec::phys

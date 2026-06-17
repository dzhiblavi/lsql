#pragma once

#include "back/exec/plan/Operation.h"
#include "back/logfmt/LogType.h"
#include "back/storage/LineSource.h"

namespace lsql::back::exec::phys {

struct LogFile {
    Arc<back::storage::LineSource> lines;
    logfmt::LogType type;
};

LogFile open(const plan::Log& log);

}  // namespace lsql::back::exec::phys

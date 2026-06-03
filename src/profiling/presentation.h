#pragma once

#include "profiling/Profiler.h"

#include <span>
#include <string>

namespace lsql::prof {

std::string formatProfile(const Profiler::Snapshot& p);
std::string formatFoldedStacks(const Profiler::Snapshot& p);
std::string formatDot(std::span<const Profiler::Snapshot> p);

}  // namespace lsql::prof

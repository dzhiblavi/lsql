#pragma once

#include "profiling/Profiler.h"

#include <span>
#include <string>

namespace lsql::prof {

std::string formatProfile(const Snapshot& p);
std::string formatFoldedStacks(const Snapshot& p);
std::string formatDot(std::span<const Snapshot> p);

}  // namespace lsql::prof

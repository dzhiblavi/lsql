#pragma once

#include "prof/Profiler.h"

#include <string>
#include <span>

namespace lsql::prof {

std::string formatProfile(const Profiler::Snapshot& p);
std::string formatFoldedStacks(const Profiler::Snapshot& p);
std::string formatDot(std::span<const Profiler::Snapshot> p);

}  // namespace lsql::prof

#pragma once

#include "util/build_info.h"

#include <string>
#include <string_view>

namespace lsql::config {

struct Language {
    static inline constexpr std::string_view LineIdentifier = "_line";
};

struct Diagnostics {
    static inline constexpr bool StackTracesEnabled = getBuildType() != BuildType::Release;
    static inline constexpr int SourceContextLines = 2;
};

std::string formatBuildSettings();

}  // namespace lsql::config

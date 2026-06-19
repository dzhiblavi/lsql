#pragma once

#include "util/build_info.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace lsql::config {

struct Language {
    static inline constexpr std::string_view LineIdentifier = "_line";
};

struct Storage {
    static inline constexpr size_t TimestampSearchBufferSize = 8 * 1024;
    static inline constexpr size_t ArchiveInputChunkSize = 64 * 1024;
    static inline constexpr size_t StreamReadChunkSize = 64 * 1024;
    static inline constexpr size_t PageSizeMultiplier = 2;
    static inline constexpr size_t CommandStderrBufferSize = 4 * 1024;
    static inline constexpr size_t CommandStderrTailSize = 16 * 1024;
};

struct Optimizer {
    static inline constexpr unsigned DefaultPasses = 5;
    static inline constexpr int CoalesceCostOverhead = 1;
    static inline constexpr int LowerCostOverhead = 1;
    static inline constexpr int SplitPartCostOverhead = 1;
    static inline constexpr int SubstrCostOverhead = 1;
    static inline constexpr int ParseTimestampCostOverhead = 10;
    static inline constexpr int UnaryOpCostOverhead = 1;
    static inline constexpr int BinaryOpCostOverhead = 1;
    static inline constexpr int CastToStringCostOverhead = 2;
    static inline constexpr int ParseStringCostOverhead = 10;
    static inline constexpr int RegexCostOverhead = 4;
    static inline constexpr int ProjectionCostOverhead = 10;
};

struct Diagnostics {
    static inline constexpr bool StackTracesEnabled = getBuildType() != BuildType::Release;
    static inline constexpr int SourceContextLines = 2;
};

std::string formatBuildSettings();

}  // namespace lsql::config

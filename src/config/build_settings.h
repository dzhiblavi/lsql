#pragma once

#include <cstddef>
#include <string_view>

#include "util/build_info.h"

namespace lsql::config {

struct Semantics {
    static inline constexpr std::string_view LineIdentifier = "_line";
};

struct Buffering {
    static constexpr inline size_t ReverseStreamBufferSize = 16 * 1024;
    static constexpr inline size_t CompressedChunkSize = 64 * 1024;
    static constexpr inline size_t DecompressedChunkSize = 64 * 1024;
};

struct IO {
    static inline constexpr size_t SystemPageSizeMultiplier = 2;
};

struct Optimize {
    static inline constexpr int CoalesceCostOverhead = 1;
    static inline constexpr int UnaryOpCostOverhead = 1;
    static inline constexpr int BinaryOpCostOverhead = 1;
    static inline constexpr int CastToStringCostOverhead = 2;
    static inline constexpr int ParseStringCostOverhead = 10;
    static inline constexpr int RegexCostOverhead = 4;
    static inline constexpr int ProjectionCostOverhead = 10;
};

struct Exceptions {
    static inline constexpr bool StackTracesEnabled = getBuildType() != BuildType::Release;
};

}  // namespace lsql::config

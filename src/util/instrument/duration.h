#pragma once

#include "util/instrument/types.h"

#include <chrono>  // IWYU pragma: keep

namespace lsql::instr {

template <typename Duration>
const char* DurationSuffix;

struct AutomaticDurationType {};
[[maybe_unused]] inline constexpr AutomaticDurationType AutomaticDuration;

std::string prettyDuration(MonotonicDuration duration);

}  // namespace lsql::instr

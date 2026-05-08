#pragma once

#include <chrono>

namespace lsql::instr {

using MonotonicClock = std::chrono::steady_clock;
using MonotonicTimePoint = typename MonotonicClock::time_point;
using MonotonicDuration = typename MonotonicClock::duration;

}  // namespace lsql::instr

#pragma once

#include "util/instrument/types.h"

namespace lsql::instr {

class Timer {
 public:
    Timer() = default;
    MonotonicDuration elapsed() const { return MonotonicClock::now() - started_at_; }

 private:
    MonotonicTimePoint started_at_ = MonotonicClock::now();
};

}  // namespace lsql::instr

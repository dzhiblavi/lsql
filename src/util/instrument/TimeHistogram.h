#pragma once

#include "util/instrument/Histogram.h"

#include <chrono>

namespace lsql::instr {

template <typename Duration = std::chrono::microseconds, size_t Bits = 31, size_t Resolution = 1>
class TimeHistogram {
 public:
 private:
    util::Histogram<Bits, Resolution> hist_;
};

}  // namespace lsql::instr

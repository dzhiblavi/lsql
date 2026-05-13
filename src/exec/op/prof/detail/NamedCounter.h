#pragma once

#include "util/instrument/Counters.h"

#include <string_view>

namespace lsql::exec::prof::detail {

struct NamedCounter {
    NamedCounter(std::string_view name, int64_t init) : init(init), name(name), counter(init) {}
    void reset() { counter.set(init); }

    int64_t init;
    std::string_view name;
    instr::Counter<int64_t> counter;
};

}  // namespace lsql::exec::prof::detail

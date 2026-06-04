#pragma once

#include "util/StrBuilder.h"
#include "util/instrument/types.h"

#include <absl/container/flat_hash_map.h>

namespace lsql::prof {

class ScopeMetricsBase {
 public:
    virtual ~ScopeMetricsBase() = default;

    virtual bool empty() const = 0;
    virtual void reset() = 0;
    virtual util::StrBuilder report() const = 0;
    virtual util::StrBuilder shortReport() const { return report(); }
    virtual std::unique_ptr<ScopeMetricsBase> clone() const = 0;

    uint64_t count = 0;
    instr::MonotonicDuration self_dur{};
    instr::MonotonicDuration total_dur{};
    absl::flat_hash_map<std::string, int64_t> counters;
};

}  // namespace lsql::prof

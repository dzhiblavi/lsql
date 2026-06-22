#pragma once

#include "util/StrBuilder.h"
#include "util/instrument/types.h"

#include <absl/container/flat_hash_map.h>

namespace lsql::prof {

struct Counters {
 public:
    Counters() = default;

    void reset() { counters_.assign(counters_.size(), 0); }

    void add(size_t id, int64_t delta) {
        if (id >= counters_.size()) [[unlikely]] {
            counters_.resize(id + 1);
        }

        counters_[id] += delta;
    }

    std::span<const int64_t> view() const { return counters_; }

 private:
    std::vector<int64_t> counters_;
};

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
    Counters counters;
};

}  // namespace lsql::prof

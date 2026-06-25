#pragma once

#include "profiling/ScopeMetrics.h"

#include "util/instrument/Histogram.h"

namespace lsql::back::exec::phys {

struct CustomScopeMetrics {
    using Duration = std::chrono::microseconds;

    void reset() {
        hist_this.reset();
        hist_total.reset();
    }

    void onExitScope(auto total, auto children) {
        hist_total.add(std::chrono::duration_cast<Duration>(total).count());
        hist_this.add(std::chrono::duration_cast<Duration>(total - children).count());
    }

    util::StrBuilder report() const {
        return util::StrBuilder()
            .line("self_hist:  {}", to_string<Duration>(hist_this))
            .line("total_hist: {}", to_string<Duration>(hist_total));
    }

    util::StrBuilder shortReport() const { return util::StrBuilder(); }

    instr::Histogram<> hist_total = {};
    instr::Histogram<> hist_this = {};
};

using ScopeMetrics = prof::ScopeMetrics<CustomScopeMetrics>;

}  // namespace lsql::back::exec::phys

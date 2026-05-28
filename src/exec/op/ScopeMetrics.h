#pragma once

#include "prof/Scope.h"

#include "util/instrument/Histogram.h"

namespace lsql::exec {

template <typename M>
concept CustomMetrics = requires(M& m, const M& mc) {
    { mc.format() } -> std::same_as<util::StrBuilder>;
    { m.reset() } -> std::same_as<void>;
};

struct EmptyCustomMetrics {
    void reset() {}
    util::StrBuilder format() const { return {}; }
};

template <CustomMetrics Custom = EmptyCustomMetrics>
struct ScopeMetrics : public prof::MetricsBase<ScopeMetrics<Custom>> {
    using Duration = std::chrono::microseconds;

    bool empty() const override { return this->count == 0; }

    void reset() override {
        this->baseReset();
        hist_total.reset();
        hist_this.reset();
        custom.reset();
    }

    void onEnterScope() {}

    void onExitScope(auto dur, auto child_dur) {
        hist_total.add(std::chrono::duration_cast<Duration>(dur).count());
        hist_this.add(std::chrono::duration_cast<Duration>(dur - child_dur).count());
    }

    util::StrBuilder report() const override {
        return this->shortReport()
            .line("self_hist:  {}", to_string<Duration>(hist_this))
            .line("total_hist: {}", to_string<Duration>(hist_total));
    }

    util::StrBuilder shortReport() const override {
        auto b = this->baseReport();
        if (auto cb = custom.format(); !cb.empty()) {
            b.block(util::StrBuilder("custom").child(cb));
        }
        return b;
    }

    util::Histogram<> hist_total = {};
    util::Histogram<> hist_this = {};
    [[no_unique_address]] Custom custom;
};

}  // namespace lsql::exec

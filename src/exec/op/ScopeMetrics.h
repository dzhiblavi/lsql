#pragma once

#include "prof/Scope.h"

#include "util/instrument/SequenceProfile.h"

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
struct ScopeMetrics : public prof::MetricsBase {
    bool empty() const override { return hist_total.empty(); }

    void reset() override {
        custom.reset();
        hist_total.reset();
        hist_this.reset();
    }

    void onEnterScope() {}

    void onExitScope(auto dur, auto child_dur) {
        hist_total.add(dur);
        hist_this.add(dur - child_dur);
    }

    util::StrBuilder report() const override {
        using util::StrBuilder;

        auto b = StrBuilder()
                     .block(StrBuilder("total: {}", hist_total.format()))
                     .block(StrBuilder("this:  {}", hist_this.format()));

        if (auto cb = custom.format(); !cb.empty()) {
            b.block(StrBuilder("custom").child(cb));
        }

        return b;
    }

    instr::SequenceProfile<std::chrono::microseconds> hist_total = {};
    instr::SequenceProfile<std::chrono::microseconds> hist_this = {};
    [[no_unique_address]] Custom custom;
};

}  // namespace lsql::exec

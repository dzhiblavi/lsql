#pragma once

#include "profiling/ScopeMetricsBase.h"

#include "util/instrument/duration.h"

namespace lsql::prof {

template <typename... Custom>
class ScopeMetrics : public ScopeMetricsBase {
 public:
    bool empty() const override { return count == 0; }

    void reset() override {
        count = 0;
        self_dur = {};
        total_dur = {};
        counters.clear();
        std::apply([&](auto&&... c) { (callReset(c), ...); }, custom_);
    }

    util::StrBuilder report() const override {
        auto b = baseReport();
        std::apply([&](auto&&... c) { (b.block(callReport(c)), ...); }, custom_);
        return b;
    }

    util::StrBuilder shortReport() const override {
        auto b = baseReport();
        std::apply([&](auto&&... c) { (b.block(callShortReport(c)), ...); }, custom_);
        return b;
    }

    void onEnterScope() {
        ++count;
        std::apply([&](auto&&... c) { (callOnEnterScope(c), ...); }, custom_);
    }

    void onExitScope(instr::MonotonicDuration total, instr::MonotonicDuration children) {
        total_dur += total;
        self_dur += total - children;
        std::apply([&](auto&&... c) { (callOnExitScope(c, total, children), ...); }, custom_);
    }

    std::unique_ptr<ScopeMetricsBase> clone() const override {
        return std::make_unique<ScopeMetrics>(*this);
    }

    template <typename C>
    C& custom() {
        return std::get<C>(custom_);
    }

 private:
    util::StrBuilder baseReport() const {
        auto b =
            util::StrBuilder()
                .line("count: {}", count)
                .line(
                    "self: total={} avg={}",
                    instr::prettyDuration(self_dur),
                    count == 0 ? "0" : instr::prettyDuration(self_dur / count))
                .line(
                    "total: total={} avg={}",
                    instr::prettyDuration(total_dur),
                    count == 0 ? "0" : instr::prettyDuration(total_dur / count));

        if (!counters.empty()) {
            auto cs = util::StrBuilder("counters");
            for (auto&& [name, value] : counters) {
                cs.item("{} = {}", name, value);
            }
            b.block(cs);
        }

        return b;
    }

    static util::StrBuilder callReport(auto& metric) {
        if constexpr (requires { metric.report(); }) {
            return metric.report();
        } else {
            return util::StrBuilder();
        }
    }

    static util::StrBuilder callShortReport(auto& metric) {
        if constexpr (requires { metric.shortReport(); }) {
            return metric.shortReport();
        } else {
            return util::StrBuilder();
        }
    }

    static void callReset(auto& metric) {
        if constexpr (requires { metric.reset(); }) {
            metric.reset();
        }
    }

    static void callOnEnterScope(auto& metric) {
        if constexpr (requires { metric.onEnterScope(); }) {
            metric.onEnterScope();
        }
    }

    static void callOnExitScope(
        auto& metric, instr::MonotonicDuration total, instr::MonotonicDuration children) {
        if constexpr (requires { metric.onExitScope(total, children); }) {
            metric.onExitScope(total, children);
        }
    }

    std::tuple<Custom...> custom_;
};

template <typename M>
concept CScopeMetrics =
    std::convertible_to<M&, ScopeMetricsBase&> && requires(M& m, instr::MonotonicDuration dur) {
        { m.onEnterScope() };
        { m.onExitScope(dur, dur) };
    };

}  // namespace lsql::prof

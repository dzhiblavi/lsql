#pragma once

#include "util/StrBuilder.h"
#include "util/instrument/duration.h"
#include "util/instrument/types.h"

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
};

template <typename... Custom>
class ScopeMetrics : public ScopeMetricsBase {
 public:
    bool empty() const override { return count == 0; }

    void reset() override {
        count = 0;
        self_dur = {};
        total_dur = {};
        std::apply([&](auto&&... c) { (callReset(c), ...); }, custom_);
    }

    util::StrBuilder report() const override {
        auto b = baseReport();
        std::apply([&](auto&&... c) { (b.item(callReport(c)), ...); }, custom_);
        return b;
    }

    util::StrBuilder shortReport() const override {
        auto b = baseReport();
        std::apply([&](auto&&... c) { (b.item(callShortReport(c)), ...); }, custom_);
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
        return util::StrBuilder()
            .line("count: {}", count)
            .line(
                "self: total={} avg={}",
                instr::prettyDuration(self_dur),
                count == 0 ? "0" : instr::prettyDuration(self_dur / count))
            .line(
                "total: total={} avg={}",
                instr::prettyDuration(total_dur),
                count == 0 ? "0" : instr::prettyDuration(total_dur / count));
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

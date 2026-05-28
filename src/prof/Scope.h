#pragma once

#include "util/NonCopyable.h"
#include "util/Pinned.h"
#include "util/StrBuilder.h"
#include "util/instrument/duration.h"
#include "util/instrument/types.h"

#include "core/verify.h"

namespace lsql::prof {

class Metrics {
 public:
    virtual ~Metrics() = default;

    virtual bool empty() const = 0;
    virtual void reset() = 0;
    virtual util::StrBuilder report() const = 0;
    virtual util::StrBuilder shortReport() const { return report(); }
    virtual std::unique_ptr<Metrics> clone() const = 0;

    uint64_t count = 0;
    instr::MonotonicDuration self_dur{};
    instr::MonotonicDuration total_dur{};
};

template <typename Self>
class MetricsBase : public Metrics {
 public:
    void baseReset() {
        count = 0;
        self_dur = {};
        total_dur = {};
    }

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

    std::unique_ptr<Metrics> clone() const override {
        return std::make_unique<Self>(*static_cast<const Self*>(this));
    }
};

template <typename M>
concept CMetrics =
    std::convertible_to<M&, Metrics&> && requires(M& m, instr::MonotonicDuration dur) {
        { m.onEnterScope() };
        { m.onExitScope(dur, dur) };
    };

class ScopeBase {
    template <CMetrics M>
    friend class Scope;

 public:
    ~ScopeBase();
    explicit ScopeBase(bool active);
    ScopeBase(ScopeBase&& rhs) noexcept;

    static ScopeBase* top();

 protected:
    instr::MonotonicTimePoint started_at_;
    instr::MonotonicDuration child_duration_ = {};
    ScopeBase* parent_;
    bool active_;
};

template <CMetrics M>
class Scope : public ScopeBase, util::NonCopyable {
 public:
    Scope() : ScopeBase(false) {}

    explicit Scope(M* metrics) : ScopeBase(true), metrics_(metrics) {
        verify(metrics != nullptr);
        ++metrics_->count;
        metrics_->onEnterScope();
    }

    Scope(Scope&& rhs) noexcept
        : ScopeBase(std::move(rhs))
        , metrics_(std::exchange(rhs.metrics_, nullptr)) {}

    ~Scope() {
        if (!active_) {
            return;
        }

        auto full_duration = instr::MonotonicClock::now() - started_at_;
        if (parent_) {
            parent_->child_duration_ += full_duration;
        }
        metrics_->total_dur += full_duration;
        metrics_->self_dur += full_duration - child_duration_;
        metrics_->onExitScope(full_duration, child_duration_);
    }

 private:
    M* metrics_ = nullptr;
};

template <CMetrics M>
class ScopeHandle : util::Pinned {
    friend class Profiler;

    template <CMetrics U>
    friend class ScopeHandle;

 public:
    ScopeHandle() = default;
    explicit ScopeHandle(M* metrics) : metrics_(metrics) {}

    operator bool() const { return metrics_ != nullptr; }
    M* metrics() const { return metrics_; }

    Scope<M> scope() {
        if (metrics_ == nullptr) {
            return {};
        }
        return Scope<M>(metrics_);
    }

 private:
    M* metrics_ = nullptr;
};

}  // namespace lsql::prof

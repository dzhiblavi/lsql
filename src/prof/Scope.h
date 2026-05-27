#pragma once

#include "util/NonCopyable.h"
#include "util/Pinned.h"
#include "util/StrBuilder.h"
#include "util/instrument/types.h"

#include "core/verify.h"

namespace lsql::prof {

class MetricsBase {
 public:
    virtual ~MetricsBase() = default;

    virtual bool empty() const = 0;
    virtual void reset() = 0;
    virtual util::StrBuilder report() const = 0;

    instr::MonotonicDuration self_dur{};
    instr::MonotonicDuration total_dur{};
};

template <typename M>
concept Metrics =
    std::derived_from<M, MetricsBase> && requires(M& m, instr::MonotonicDuration dur) {
        { m.onEnterScope() };
        { m.onExitScope(dur, dur) };
    };

class ScopeBase {
    template <Metrics M>
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

template <Metrics M>
class Scope : public ScopeBase, util::NonCopyable {
 public:
    Scope() : ScopeBase(false) {}

    explicit Scope(M* metrics) : ScopeBase(true), metrics_(metrics) {
        verify(metrics != nullptr);
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

template <Metrics M>
class ScopeHandle : util::Pinned {
    friend class Profiler;

    template <Metrics U>
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

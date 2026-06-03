#pragma once

#include "profiling/ScopeMetrics.h"

#include "util/NonCopyable.h"
#include "util/Pinned.h"
#include "util/instrument/types.h"

#include "util/verify.h"

namespace lsql::prof {

class ScopeBase {
    template <CScopeMetrics M>
    friend class Scope;

 public:
    ~ScopeBase();
    explicit ScopeBase(bool active);
    ScopeBase(ScopeBase&& rhs) noexcept;

 protected:
    instr::MonotonicTimePoint started_at_;
    instr::MonotonicDuration child_duration_ = {};
    ScopeBase* parent_;
    bool active_;
};

template <CScopeMetrics M>
class Scope : public ScopeBase, util::NonCopyable {
 public:
    Scope() : ScopeBase(false) {}

    explicit Scope(M* metrics) : ScopeBase(true), metrics_(metrics) {
        verify_dbg(metrics != nullptr);
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
        metrics_->onExitScope(full_duration, child_duration_);
    }

 private:
    M* metrics_ = nullptr;
};

template <CScopeMetrics M>
class ScopeHandle : util::Pinned {
    friend class Profiler;

    template <CScopeMetrics U>
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

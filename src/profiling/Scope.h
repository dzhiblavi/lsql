#pragma once

#include "profiling/ScopeBase.h"
#include "profiling/ScopeMetrics.h"

#include "util/NonCopyable.h"
#include "util/verify.h"

namespace lsql::prof {

template <CScopeMetrics M>
class Scope : public ScopeBase, util::NonCopyable {
 public:
    Scope() : ScopeBase(false, nullptr) {}

    explicit Scope(M* m) : ScopeBase(true, m) {
        verify_dbg(m != nullptr);
        metrics()->onEnterScope();
    }

    Scope(Scope&& rhs) noexcept : ScopeBase(std::move(rhs)) {}

    ~Scope() {
        if (!active_) {
            return;
        }

        auto full_duration = instr::MonotonicClock::now() - started_at_;
        if (parent_) {
            parent_->addChildDuration(full_duration);
        }
        metrics()->onExitScope(full_duration, child_duration_);
    }

 private:
    M* metrics() { return static_cast<M*>(this->metrics_); }
};

template <CScopeMetrics M>
class ScopeHandle : public ScopeHandleBase {
 public:
    ScopeHandle() = default;
    explicit ScopeHandle(M* metrics) : ScopeHandleBase(metrics) {}

    M* metrics() const { return getMetrics(); }

    Scope<M> scope() {
        if (metrics_ == nullptr) {
            return {};
        }
        return Scope<M>(getMetrics());
    }

 private:
    M* getMetrics() const { return static_cast<M*>(metrics_); }
};

}  // namespace lsql::prof

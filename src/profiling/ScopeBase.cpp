#include "profiling/ScopeBase.h"

#include "util/verify.h"

namespace lsql::prof {

namespace {

thread_local ScopeBase* this_thread_top_ = nullptr;

}  // namespace

ScopeBase::ScopeBase(bool active, ScopeMetricsBase* metrics) : active_(active) {
    if (!active) {
        return;
    }

    started_at_ = instr::MonotonicClock::now();
    parent_ = std::exchange(this_thread_top_, this);
    metrics_ = metrics;
}

ScopeBase::~ScopeBase() {
    if (!active_) {
        return;
    }

    verify_dbg(this_thread_top_ == this);
    this_thread_top_ = parent_;
}

ScopeBase::ScopeBase(ScopeBase&& rhs) noexcept
    : started_at_(rhs.started_at_)
    , child_duration_(rhs.child_duration_)
    , parent_(rhs.parent_)
    , metrics_(std::exchange(rhs.metrics_, nullptr))
    , active_(std::exchange(rhs.active_, false)) {
    if (active_) {
        verify_dbg(this_thread_top_ == &rhs);
        this_thread_top_ = this;
    }
}

void ScopeBase::addChildDuration(instr::MonotonicDuration dur) {
    child_duration_ += dur;
}

ScopeMetricsBase& ScopeBase::metrics() {
    verify_dbg(metrics_ != nullptr);
    return *metrics_;
}

ScopeBase* ScopeBase::current() {
    return this_thread_top_;
}

}  // namespace lsql::prof

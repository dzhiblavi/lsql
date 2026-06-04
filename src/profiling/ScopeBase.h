#pragma once

#include "profiling/ScopeMetricsBase.h"

#include "util/instrument/types.h"

namespace lsql::prof {

class ScopeBase {
 public:
    ~ScopeBase();
    ScopeBase(bool active, ScopeMetricsBase* metrics);
    ScopeBase(ScopeBase&& rhs) noexcept;

    void addChildDuration(instr::MonotonicDuration);
    ScopeMetricsBase& metrics();
    static ScopeBase* current();

 protected:
    instr::MonotonicTimePoint started_at_ = {};
    instr::MonotonicDuration child_duration_ = {};
    ScopeBase* parent_ = nullptr;
    ScopeMetricsBase* metrics_ = nullptr;
    bool active_ = false;
};

class ScopeHandleBase {
    friend class Profiler;

 public:
    ScopeHandleBase() = default;
    explicit ScopeHandleBase(ScopeMetricsBase* m) : metrics_(m) {}
    operator bool() const { return metrics_ != nullptr; }

 protected:
    ScopeMetricsBase* metrics_;
};

}  // namespace lsql::prof

#pragma once

#include "profiling/Profiler.h"

namespace lsql::prof {

void setGlobalProfiler(Profiler* prof);
Profiler* globalProfiler();

template <CScopeMetrics M>
ScopeHandle<M> newScope(std::string name) {
    if (auto prof = globalProfiler()) {
        return prof->newScope<M>(std::move(name));
    }

    return {};
}

template <CScopeMetrics M, typename... Args>
ScopeHandle<M> newScope(std::format_string<const Args&...> fmt, const Args&... args) {
    return newScope<M>(std::format(fmt, args...));
}

inline void addEdge(prof::ScopeMetricsBase* parent, prof::ScopeMetricsBase* child) {
    if (auto prof = globalProfiler()) {
        return prof->addEdge(parent, child);
    }
}

template <CScopeMetrics P, CScopeMetrics C>
void addEdge(ScopeHandle<P>* parent, ScopeHandle<C>* child) {
    if (auto prof = globalProfiler()) {
        return prof->addEdge(parent->metrics(), child->metrics());
    }
}

inline void reset() {
    if (auto prof = globalProfiler()) {
        prof->reset();
    }
}

}  // namespace lsql::prof

#pragma once

#include "prof/Profiler.h"

namespace lsql::prof {

void setGlobalProfiler(Profiler* prof);
Profiler* globalProfiler();

template <CMetrics M>
ScopeHandle<M> newScope(std::string name) {
    if (auto prof = globalProfiler()) {
        return prof->newScope<M>(std::move(name));
    }

    return {};
}

template <CMetrics M, typename... Args>
ScopeHandle<M> newScope(std::format_string<const Args&...> fmt, const Args&... args) {
    return newScope<M>(std::format(fmt, args...));
}

inline void addEdge(prof::Metrics* parent, prof::Metrics* child) {
    if (auto prof = globalProfiler()) {
        return prof->addEdge(parent, child);
    }
}

template <CMetrics P, CMetrics C>
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

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

inline void addEdge(ScopeHandleBase parent, ScopeHandleBase child) {
    if (auto prof = globalProfiler()) {
        return prof->addEdge(parent, child);
    }
}

inline void reset() {
    if (auto prof = globalProfiler()) {
        prof->reset();
    }
}

inline void addCounter(std::string_view name, int64_t delta = 1) {
    if (auto scope = ScopeBase::current()) {
        scope->metrics().counters[name] += delta;
    }
}

}  // namespace lsql::prof

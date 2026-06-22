#pragma once

#include "profiling/Counters.h"
#include "profiling/Profiler.h"

namespace lsql::prof {

void setGlobalProfiler(Profiler* prof);
Profiler* globalProfiler();

void reset();
void addEdge(ScopeHandleBase parent, ScopeHandleBase child);
void addCounter(CounterId id, int64_t delta = 1);

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

}  // namespace lsql::prof

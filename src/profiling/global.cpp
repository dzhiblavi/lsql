#include "profiling/global.h"

namespace lsql::prof {

namespace {

Profiler* global_ = nullptr;

}  // namespace

void setGlobalProfiler(Profiler* prof) {
    global_ = prof;
}

Profiler* globalProfiler() {
    return global_;
}

void addEdge(ScopeHandleBase parent, ScopeHandleBase child) {
    if (auto prof = globalProfiler()) {
        return prof->addEdge(parent, child);
    }
}

void reset() {
    if (auto prof = globalProfiler()) {
        prof->reset();
    }
}

void addCounter(std::string_view name, int64_t delta) {
    if (auto scope = ScopeBase::current()) {
        scope->metrics().counters[name] += delta;
    }
}

}  // namespace lsql::prof

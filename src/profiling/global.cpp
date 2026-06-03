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

}  // namespace lsql::prof

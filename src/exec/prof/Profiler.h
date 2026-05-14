#pragma once

#include "exec/prof/OperationHandle.h"

#include <unordered_map>

namespace lsql::exec::prof {

class Profiler {
 public:
    explicit Profiler(size_t num_threads);
    ~Profiler();

    static OperationHandle registerOperation(const Operation* self);
    static std::string report();
    static void reset();

 private:
    static Profiler* profiler();

    OperationHandle registerOperationImpl(const Operation* self);
    std::string reportImpl();
    void resetImpl();

    const size_t num_threads_;
    std::unordered_map<const Operation*, detail::OperationStats> stats_;
};

}  // namespace lsql::exec::prof

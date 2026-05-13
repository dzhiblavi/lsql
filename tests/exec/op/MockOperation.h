#pragma once

#include "exec/op/Operation.h"

namespace lsql::exec {

struct MockOperation : Operation {
    MockOperation() : Operation(0, "MockOperation") {}

    ExplanationItem explain(ExplanationCtx /*ctx*/) const override { return {}; }
    void init(int out_phase) override { init_calls.push_back(out_phase); }
    void push(int phase, const Record* record) { emit(phase, record); }

    std::vector<int> init_calls;
};

}  // namespace lsql::exec

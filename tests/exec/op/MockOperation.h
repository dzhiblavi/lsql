#pragma once

#include "exec/op/OperationBase.h"

namespace lsql::exec {

struct MockOperation : OperationBase<MockOperation> {
    MockOperation() : OperationBase(0, "MockOperation") {}

    ExplanationItem explain(ExplanationCtx /*ctx*/) const override { return {}; }
    void init(int out_phase) override { init_calls.push_back(out_phase); }
    bool push(int phase, const Record* record) { return emit(phase, record); }

    std::vector<int> init_calls;
};

}  // namespace lsql::exec

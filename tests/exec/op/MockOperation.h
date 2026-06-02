#pragma once

#include "back/exec/op/OperationBase.h"

namespace lsql::back::exec {

struct MockOperation : OperationBase<MockOperation> {
    explicit MockOperation(ConstFieldBindingPtr binding) : OperationBase(0, binding) {}

    void init(int out_phase, const FieldSet& /*fields*/) override {
        init_calls.push_back(out_phase);
    }

    ExplanationItem explain(ExplanationCtx /*ctx*/) const override { return {}; }

    bool push(int phase, const Record* record) { return emit(phase, record); }

    std::vector<int> init_calls;
};

}  // namespace lsql::back::exec

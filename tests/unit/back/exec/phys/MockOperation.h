#pragma once

#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

struct MockOperation : OperationBase<MockOperation> {
    MockOperation() : OperationBase(0) {}

    bool push(const Record* record) { return emit(record); }

    std::vector<int> init_calls;
};

}  // namespace lsql::back::exec::phys

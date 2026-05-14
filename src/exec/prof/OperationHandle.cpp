#include "exec/prof/OperationHandle.h"

#include <stack>

namespace lsql::exec::prof {

namespace {

thread_local std::stack<OperationHandle*> operations_;

}  // namespace

void pushCurrentOperation(OperationHandle* handle) {
    operations_.push(handle);
}

void popCurrentOperation(OperationHandle* handle) {
    verify(!operations_.empty());
    verify(operations_.top() == handle);
    operations_.pop();
}

OperationHandle& currentOperation() {
    thread_local OperationHandle none;
    return operations_.empty() ? none : *operations_.top();
}

}  // namespace lsql::exec::prof

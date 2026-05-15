#include "exec/prof/OperationHandle.h"

namespace lsql::exec::prof {

namespace {

thread_local util::IntrusiveForwardList<OperationHandle> operations_;

}  // namespace

void pushCurrentOperation(OperationHandle* handle) {
    operations_.push_front(*handle);
}

void popCurrentOperation(OperationHandle* handle) {
    verify(!operations_.empty());
    verify(&operations_.front() == handle);
    operations_.pop_front();
}

OperationHandle& currentOperation() {
    thread_local OperationHandle none;
    return operations_.empty() ? none : operations_.front();
}

}  // namespace lsql::exec::prof

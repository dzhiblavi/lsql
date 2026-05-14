#include "exec/prof/OperationHandle.h"

namespace lsql::exec::prof {

namespace {

thread_local OperationHandle* current = nullptr;

}  // namespace

OperationHandle& currentOperation() {
    thread_local OperationHandle none;
    return current == nullptr ? none : *current;
}

}  // namespace lsql::exec::prof

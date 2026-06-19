#pragma once

#include "core/function/Aggregate.h"
#include "core/function/Executor.h"
#include "core/function/Function.h"

namespace lsql::func {

Arc<Executor> buildScalar(const Function& func);
Arc<Aggregate> buildAggregate(const Function& func);

}  // namespace lsql::func

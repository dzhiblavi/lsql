#pragma once

#include "back/exec/expr/Aggregate.h"
#include "back/exec/expr/Scalar.h"

#include <vector>

namespace lsql::back::exec {

using SortList = std::vector<ScalarPtr>;
using SortKey = std::vector<Value>;

struct AggregateProjector {
    FieldId field_id;
    AggregatePtr expr;
};

using AggregateProjectorPtr = std::unique_ptr<AggregateProjector>;
using AggregateProjectionList = std::vector<std::unique_ptr<AggregateProjector>>;
using AggregateProjectionMap = std::unordered_map<FieldId, AggregateProjector*>;

struct ScalarProjector {
    FieldId field_id;
    ScalarPtr expr;
};

using ScalarProjectorPtr = std::unique_ptr<ScalarProjector>;
using ScalarProjectionList = std::vector<std::unique_ptr<ScalarProjector>>;
using ScalarProjectionMap = std::unordered_map<FieldId, ScalarProjector*>;

}  // namespace lsql::back::exec

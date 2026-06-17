#pragma once

#include "back/exec/expr/Aggregate.h"
#include "back/exec/expr/Scalar.h"
#include "back/exec/plan/fwd/Operation.h"

#include "core/schema/Schema.h"
#include "core/types.h"
#include "util/uniq_id.h"

#include <map>
#include <vector>

namespace lsql::back::exec::plan {

struct ScalarProjector {
    FieldId field_id;
    ScalarPtr expr;
};

struct AggregateProjector {
    FieldId field_id;
    AggregatePtr expr;
};

struct Aggregate {
    Arc<Operation> source;
    std::vector<Arc<AggregateProjector>> aggregates;
};

struct Filter {
    Arc<Operation> source;
    ScalarPtr condition;
};

struct Group {
    Arc<Operation> source;
    std::vector<Arc<AggregateProjector>> aggregates;
    std::vector<Arc<ScalarProjector>> group_key;
};

struct Limit {
    Arc<Operation> source;
    int limit;
};

struct Log {
    std::string path;
    std::optional<TimeRange> range;
};

struct MarkJoin {
    Arc<Operation> match;
    Arc<Operation> source;
    ScalarPtr scalar;
    FieldId match_field_id;
    FieldId output_field_id;
};

struct Materialize {
    Arc<Operation> source;
};

struct MergeSorted {
    Arc<Operation> left;
    Arc<Operation> right;
    std::vector<ScalarPtr> sort_key;
    bool desc;
};

struct Projection {
    Arc<Operation> source;
    std::vector<Arc<ScalarProjector>> projectors;
};

struct SemiJoin {
    Arc<Operation> match;
    Arc<Operation> source;
    ScalarPtr scalar;
    FieldId match_field_id;
};

struct Sort {
    Arc<Operation> source;
    std::vector<ScalarPtr> sort_key;
    bool desc;
};

struct TopK {
    Arc<Operation> source;
    std::vector<ScalarPtr> sort_key;
    bool desc;
    int top_count;
};

struct UnionAll {
    Arc<Operation> left;
    Arc<Operation> right;
};

struct Values {
    std::vector<Value> values;
};

struct Operation {
    OperationNode node;

    int id = util::uniqId();
    int max_phase = -1;
    int min_phase;
    Schema schema;
    std::map<int, FieldSet> required_fields = {};
};

}  // namespace lsql::back::exec::plan

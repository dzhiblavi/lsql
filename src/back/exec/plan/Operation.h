#pragma once

#include "back/exec/expr/Aggregate.h"
#include "back/exec/expr/Scalar.h"
#include "back/exec/expr/ScalarContext.h"
#include "back/exec/plan/fwd/Operation.h"

#include "core/schema/Schema.h"
#include "core/time_formats.h"
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
    Arc<ScalarContext> scalar_context;
};

struct Filter {
    Arc<Operation> source;
    ScalarPtr condition;
    Arc<ScalarContext> scalar_context;
};

struct Group {
    Arc<Operation> source;
    std::vector<Arc<AggregateProjector>> aggregates;
    std::vector<Arc<ScalarProjector>> group_key;
    Arc<ScalarContext> scalar_context;
};

struct Limit {
    Arc<Operation> source;
    int limit;
};

struct Log {
    std::string path;
    std::optional<TimeRange> range;
    std::optional<TimeFormat> time_format;
};

struct Stream {
    std::string command;
};

struct HashMarkJoin {
    Arc<Operation> match;
    Arc<Operation> source;
    ScalarPtr scalar;
    FieldId match_field_id;
    FieldId output_field_id;
};

struct HashLeftJoin {
    Arc<Operation> left;
    Arc<Operation> right;
    ScalarPtr left_key;
    ScalarPtr right_key;
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
    Arc<ScalarContext> scalar_context;
};

struct HashSemiJoin {
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
    std::vector<Arc<Operation>> sources;
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

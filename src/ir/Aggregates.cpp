#include "ir/Aggregates.h"

#include "ir/Scalars.h"
#include "util/equal.h"
#include "util/hash.h"

namespace lsql::ir {

bool operator==(const Aggregate& left, const Aggregate& right) {
    return util::equal(left.node, right.node) && left.output_field_id == right.output_field_id &&
           left.value_type == right.value_type;
}

}  // namespace lsql::ir

size_t std::hash<lsql::ir::Aggregate>::operator()(const lsql::ir::Aggregate& aggregate) const {
    size_t seed = 0;
    lsql::util::hashCombine(seed, lsql::util::hash(aggregate.node));
    lsql::util::hashCombine(seed, lsql::util::hash(aggregate.output_field_id));
    lsql::util::hashCombine(seed, lsql::util::hash(aggregate.value_type));
    return seed;
}

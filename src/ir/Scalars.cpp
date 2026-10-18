#include "ir/Scalars.h"

#include "util/equal.h"
#include "util/hash.h"

namespace lsql::ir {

bool operator==(const Scalar& left, const Scalar& right) {
    return util::equal(left.node, right.node) && left.value_type == right.value_type;
}

}  // namespace lsql::ir

size_t std::hash<lsql::ir::Scalar>::operator()(const lsql::ir::Scalar& scalar) const {
    size_t seed = 0;
    lsql::util::hashCombine(seed, lsql::util::hash(scalar.node));
    lsql::util::hashCombine(seed, lsql::util::hash(scalar.value_type));
    return seed;
}

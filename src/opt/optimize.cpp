#include "opt/optimize.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "opt/aggregate_fold.h"
#include "opt/const_fold.h"
#include "opt/relation_simplify.h"
#include "opt/scalar_simplify.h"

namespace lsql::opt {

ir::Program optimize(ir::Program program) {
    program = constFold(std::move(program));
    program = scalarSimplify(std::move(program));
    program = constFold(std::move(program));
    program = aggregateFold(std::move(program));
    program = relationSimplify(std::move(program));
    return program;
}

}  // namespace lsql::opt

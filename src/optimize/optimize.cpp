#include "optimize/optimize.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "optimize/aggregate_fold.h"
#include "optimize/const_fold.h"
#include "optimize/empty_relation_prune.h"
#include "optimize/projection_collapse.h"
#include "optimize/relation_simplify.h"
#include "optimize/scalar_simplify.h"

namespace lsql::opt {

ir::Program optimize(ir::Program program, Context& ctx) {
    ctx.nextPass();
    program = constFold(std::move(program), ctx);
    program = scalarSimplify(std::move(program), ctx);
    program = constFold(std::move(program), ctx);
    program = aggregateFold(std::move(program), ctx);
    program = relationSimplify(std::move(program), ctx);
    program = projectionCollapse(std::move(program), ctx);
    program = emptyRelationPrune(std::move(program), ctx);
    return program;
}

}  // namespace lsql::opt

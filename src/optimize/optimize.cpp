#include "optimize/optimize.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "optimize/const_aggregate_fold.h"
#include "optimize/const_scalar_fold.h"
#include "optimize/empty_relation_prune.h"
#include "optimize/limit_pushdown.h"
#include "optimize/projection_collapse.h"
#include "optimize/relation_simplify.h"
#include "optimize/scalar_simplify.h"

namespace lsql::opt {

ir::Program optimize(ir::Program program, Context& ctx) {
    ctx.nextPass();
    program = constScalarFold(std::move(program), ctx);
    program = scalarSimplify(std::move(program), ctx);
    program = constScalarFold(std::move(program), ctx);
    program = constAggregateFold(std::move(program), ctx);
    program = relationSimplify(std::move(program), ctx);
    program = limitPushdown(std::move(program), ctx);
    program = projectionCollapse(std::move(program), ctx);
    program = emptyRelationPrune(std::move(program), ctx);
    return program;
}

}  // namespace lsql::opt

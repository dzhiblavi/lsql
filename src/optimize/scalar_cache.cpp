#include "optimize/scalar_cache.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"
#include "optimize/ScalarCostEstimator.h"
#include "util/TreeWalker.h"
#include "util/deep_copy.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace lsql::opt {

namespace {

struct CacheStripper : util::TreeWalker<CacheStripper> {
    using Base = util::TreeWalker<CacheStripper>;
    using Base::visit;

    void visit(ir::Scalar& scalar) {
        while (auto* cached = std::get_if<ir::CachedScalar>(&scalar.node)) {
            auto source = std::move(*cached->scalar);
            scalar = std::move(source);
        }
        Base::visit(scalar);
    }
};

struct RepeatedExpression {
    const ir::Scalar* scalar;
    size_t count;
    int64_t cost;
};

struct ExpressionCollector : util::TreeWalker<ExpressionCollector> {
    using Base = util::TreeWalker<ExpressionCollector>;
    using Base::visit;

    const ScalarCostEstimator& estimator;
    std::unordered_map<ir::Scalar, size_t> indices;
    std::vector<RepeatedExpression> expressions;

    void visit(const ir::Scalar& scalar) {
        if (std::holds_alternative<ir::FieldScalar>(scalar.node) ||
            std::holds_alternative<ir::ValueScalar>(scalar.node)) {
            return;
        }

        auto [entry, inserted] = indices.try_emplace(util::deepCopy(scalar), expressions.size());
        if (inserted) {
            expressions.push_back(
                {.scalar = &entry->first, .count = 1, .cost = estimator.cost(scalar)});
        } else {
            ++expressions[entry->second].count;
        }
        Base::visit(scalar);
    }
};

struct ExpressionCacher : util::TreeWalker<ExpressionCacher> {
    using Base = util::TreeWalker<ExpressionCacher>;
    using Base::visit;

    const ir::Scalar& expression;

    void visit(ir::Scalar& scalar) {
        if (std::holds_alternative<ir::FieldScalar>(scalar.node) ||
            std::holds_alternative<ir::ValueScalar>(scalar.node)) {
            return;
        }

        auto stripped = util::deepCopy(scalar);
        CacheStripper stripper;
        stripper.visit(stripped);
        if (stripped == expression) {
            auto type = scalar.value_type;
            auto source = box(std::move(scalar));
            scalar = {.node = ir::CachedScalar{.scalar = std::move(source)}, .value_type = type};
            return;
        }
        Base::visit(scalar);
    }
};

int64_t projectorsCost(const std::vector<ir::Projector>& projectors) {
    ScalarCostEstimator estimator;
    for (const auto& projector : projectors) {
        estimator.estimate(*projector.expr);
    }
    return estimator.totalCost();
}

void stripCache(std::vector<ir::Projector>& projectors) {
    CacheStripper stripper;
    stripper.visit(projectors);
}

void buildCache(std::vector<ir::Projector>& projectors) {
    ScalarCostEstimator estimator;
    for (const auto& projector : projectors) {
        estimator.estimate(*projector.expr);
    }

    ExpressionCollector collector{.estimator = estimator, .indices = {}, .expressions = {}};
    collector.visit(std::as_const(projectors));
    std::stable_sort(
        collector.expressions.begin(),
        collector.expressions.end(),
        [](const auto& left, const auto& right) { return left.cost > right.cost; });

    auto cost = estimator.totalCost();
    auto overhead = config::optimizer().get(config::OptimizerSetting::CachedScalarCostOverhead);

    for (const auto& expression : collector.expressions) {
        auto count = int64_t(expression.count);
        if (count < 2 || expression.cost * (count - 1) <= overhead * count) {
            continue;
        }

        auto candidate = util::deepCopy(projectors);

        ExpressionCacher cacher{.expression = *expression.scalar};
        cacher.visit(candidate);

        auto candidate_cost = projectorsCost(candidate);
        if (candidate_cost < cost) {
            projectors = std::move(candidate);
            cost = candidate_cost;
        }
    }
}

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    void visit(ir::ProjectionRelation& projection) {
        Base::visit(projection);

        auto current_cost = projectorsCost(projection.projectors);
        auto candidate = util::deepCopy(projection.projectors);

        stripCache(candidate);
        buildCache(candidate);
        auto candidate_cost = projectorsCost(candidate);

        if (candidate_cost < current_cost) {
            projection.projectors = std::move(candidate);
            ctx.setChanges().note(
                "projection scalar cache updated, cost: new={}, old={}",
                candidate_cost,
                current_cost);
        }
    }
};

}  // namespace

ir::Program scalarCache(ir::Program program, Context& ctx) {
    Optimizer optimizer{.ctx = ctx};
    for (auto& statement : program.statements) {
        optimizer.visit(statement);
    }
    return program;
}

}  // namespace lsql::opt

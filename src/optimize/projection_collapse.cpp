#include "optimize/projection_collapse.h"

#include "optimize/ScalarCostEstimator.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "util/TreeWalker.h"
#include "util/logging.h"

#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct CloneScalarView : util::TreeWalker<CloneScalarView> {
    using Base = util::TreeWalker<CloneScalarView>;
    using Base::visit;

    virtual ~CloneScalarView() = default;

    void visit(const ir::Scalar& s) {
        Base::visit(s);
        auto scalar = std::visit([&](const auto& node) { return view(node, s); }, s.node);
        verify(scalars_.emplace(&s, std::move(scalar)).second);
    }

    virtual ir::Scalar view(const ir::FieldScalar& s, const ir::Scalar& self) {
        return {.node = s, .value_type = self.value_type};
    }

    ir::Scalar view(const ir::ValueScalar& s, const ir::Scalar& self) {
        return ir::Scalar{.node = s, .value_type = self.value_type};
    }

    ir::Scalar view(const ir::CoalesceScalar& s, const ir::Scalar& self) {
        return ir::Scalar{
            .node = ir::CoalesceScalar{.args = takeArgs(s.args)},
            .value_type = self.value_type,
        };
    }

    ir::Scalar view(const ir::FnCallScalar& s, const ir::Scalar& self) {
        return ir::Scalar{
            .node = ir::FnCallScalar{.function = s.function, .args = takeArgs(s.args)},
            .value_type = self.value_type,
        };
    }

    ir::Scalar clone(const ir::Scalar& s) {
        visit(s);
        return takeScalar(&s);
    }

    ir::Scalar view(const ir::CachedScalar& s, const ir::Scalar& self) {
        return ir::Scalar{
            .node = ir::CachedScalar{.scalar = box(takeScalar(s.scalar.get()))},
            .value_type = self.value_type,
        };
    }

 private:
    ir::Scalar takeScalar(const ir::Scalar* s) {
        auto scalar = scalars_.extract(s);
        verify(!scalar.empty());
        return std::move(scalar.mapped());
    }

    std::vector<ir::Scalar> takeArgs(const std::vector<ir::Scalar>& args) {
        std::vector<ir::Scalar> result;
        result.reserve(args.size());
        for (const auto& arg : args) {
            result.push_back(takeScalar(&arg));
        }
        return result;
    }

    std::unordered_map<const ir::Scalar*, ir::Scalar> scalars_;
};

struct ScalarCollapser : CloneScalarView {
    CloneScalarView clone;
    std::unordered_map<FieldId, const ir::Scalar*> inner;

    ir::Scalar view(const ir::FieldScalar& s, const ir::Scalar& /*self*/) override {
        auto it = inner.find(s.field_id);
        verify(it != inner.end());
        return clone.clone(*it->second);
    }

    ir::Scalar collapse(const ir::Scalar& s) { return CloneScalarView::clone(s); }
};

struct Optimizer : util::TreeWalker<Optimizer> {
    using Base = util::TreeWalker<Optimizer>;
    using Base::visit;

    Context& ctx;

    bool isProjectionProjection(const ir::ProjectionRelation& p) {
        return std::holds_alternative<ir::ProjectionRelation>(p.source->node);
    }

    void optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (!isProjectionProjection(rel)) {
            return;
        }

        auto& inner = std::get<ir::ProjectionRelation>(rel.source->node);

        ScalarCostEstimator inner_estimator;
        for (const auto& projector : inner.projectors) {
            inner_estimator.estimate(*projector.expr);
        }
        ScalarCostEstimator outer_estimator;
        for (const auto& projector : rel.projectors) {
            outer_estimator.estimate(*projector.expr);
        }

        std::unordered_map<FieldId, const ir::Scalar*> inner_scalars;
        inner_scalars.reserve(inner.projectors.size());
        for (auto&& [id, scalar] : inner.projectors) {
            inner_scalars.emplace(id, scalar.get());
        }

        ScalarCollapser collapser;
        collapser.inner = std::move(inner_scalars);

        std::vector<ir::Projector> collapsed;
        collapsed.reserve(rel.projectors.size());
        for (auto&& [id, scalar] : rel.projectors) {
            collapsed.push_back({
                .alias_field_id = id,
                .expr = box(collapser.collapse(*scalar)),
            });
        }

        ScalarCostEstimator collapsed_estimator;
        for (const auto& projector : collapsed) {
            collapsed_estimator.estimate(*projector.expr);
        }
        auto current_cost =
            inner_estimator.totalCost() + outer_estimator.totalCost() +
            config::optimizer().get(config::OptimizerSetting::ProjectionCostOverhead);
        if (collapsed_estimator.totalCost() > current_cost) {
            llog::trace(
                "projection collapse skipped, cost too high: collapsed={}, old={}",
                collapsed_estimator.totalCost(),
                current_cost);
            return;
        }
        ctx.setChanges().note(
            "projection collapsed, cost: collapsed={}, old={}",
            collapsed_estimator.totalCost(),
            current_cost);

        self.node = ir::ProjectionRelation{
            .source = std::move(inner.source),
            .projectors = std::move(collapsed),
        };
    }

    void visit(ir::Relation& self) {
        Base::visit(self);

        util::match(self.node, [&](auto& node) {
            if constexpr (requires { optimize(node, self); }) {
                auto name = rfl::type_name_t<decltype(node)>().name();
                llog::trace("applying projection_collapse step for {}", name);
                optimize(node, self);
            }
        });
    }
};

}  // namespace

ir::Program projectionCollapse(ir::Program program, Context& ctx) {
    Optimizer opt{.ctx = ctx};
    for (auto& statement : program.statements) {
        opt.visit(statement);
    }

    return program;
}

}  // namespace lsql::opt

#include "optimize/projection_collapse.h"
#include "ir/pass.h"

#include "config/build_settings.h"

#include <llog/log.h>
#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct ScalarCostEstimator : ir::ScalarViewPass<ScalarCostEstimator, int> {
    using Optimizer = config::Optimizer;

    virtual ~ScalarCostEstimator() = default;

    int cost(FieldId id) const {
        auto it = cost_.find(id);
        verify(it != cost_.end());
        return it->second;
    }

    virtual int view(const ir::FieldScalar&, const ir::Scalar& self) = 0;

    int view(const ir::ValueScalar&, auto&) { return 0; }

    int view(const ir::CoalesceScalar& /*s*/, auto& /*self*/, auto args) {
        int cost = std::ranges::fold_left(args, 0, std::plus());
        return cost + Optimizer::CoalesceCostOverhead;
    }

    int view(const ir::FnCallScalar& s, auto&, auto args) {
        int cost = std::ranges::fold_left(args, 0, std::plus());

        util::match(
            s.function,
            util::Overloaded{
                [&](const func::Coalesce&) { cost += Optimizer::CoalesceCostOverhead; },
                [&](const func::Cast& f) {
                    verify_dbg(s.args.size() == 1);

                    if (f.cast_to == ValueType::String) {
                        cost += Optimizer::CastToStringCostOverhead;
                    }
                    if (s.args.front().value_type == ValueType::String) {
                        cost += Optimizer::ParseStringCostOverhead;
                    }
                },
                [&](const func::Like&) { cost += Optimizer::RegexCostOverhead; },
                [&](const func::RSubstr&) { cost += Optimizer::RegexCostOverhead; },
                [&](const func::BooleanNegate&) { cost += Optimizer::UnaryOpCostOverhead; },
                [&](const func::Equal&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::NotEqual&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::And&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Or&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Divide&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Add&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Subtract&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const auto&) {},
            });

        return cost;
    }

    void estimateProjector(FieldId id, const auto& s) {
        int cost = pass(s);
        cost_[id] = cost;
        total_cost_ += cost;
    }

    int totalCost() const { return total_cost_; }

 protected:
    int total_cost_ = 0;
    std::unordered_map<FieldId, int> cost_;
};

struct RawScalarCostEstimator : ScalarCostEstimator {
    int view(const ir::FieldScalar&, const ir::Scalar&) override { return 0; }
};

struct ScalarCollapseCostEstimator : ScalarCostEstimator {
    const ScalarCostEstimator* inner = nullptr;

    int view(const ir::FieldScalar& outer, const ir::Scalar&) override {
        return inner->cost(outer.field_id);
    }
};

struct CloneScalarView : ir::ScalarViewPass<CloneScalarView, ir::Scalar> {
    virtual ~CloneScalarView() = default;

    virtual ir::Scalar view(const ir::FieldScalar& s, const ir::Scalar& self) {
        return {.node = s, .value_type = self.value_type};
    }

    ir::Scalar view(const ir::ValueScalar& s, auto& self) {
        return ir::Scalar{.node = s, .value_type = self.value_type};
    }

    ir::Scalar view(const ir::CoalesceScalar& /*s*/, auto& self, auto args) {
        return ir::Scalar{
            .node = ir::CoalesceScalar{.args = std::move(args)},
            .value_type = self.value_type,
        };
    }

    ir::Scalar view(const ir::FnCallScalar& s, auto& self, auto args) {
        return ir::Scalar{
            .node = ir::FnCallScalar{.function = s.function, .args = std::move(args)},
            .value_type = self.value_type,
        };
    }

    ir::Scalar clone(const ir::Scalar& s) { return pass(s); }
};

struct ScalarCollapser : CloneScalarView {
    CloneScalarView clone;
    std::unordered_map<FieldId, const ir::Scalar*> inner;

    ir::Scalar view(const ir::FieldScalar& s, const ir::Scalar& /*self*/) override {
        auto it = inner.find(s.field_id);
        verify(it != inner.end());
        return clone.clone(*it->second);
    }

    ir::Scalar collapse(const ir::Scalar& s) { return pass(s); }
};

struct Optimizer : ir::ConsumePass<Optimizer> {
    Context& ctx;

    bool isProjectionProjection(const ir::ProjectionRelation& p) {
        return std::holds_alternative<ir::ProjectionRelation>(p.source->node);
    }

    ir::Relation optimize(ir::ProjectionRelation& rel, ir::Relation& self) {
        if (!isProjectionProjection(rel)) {
            return std::move(self);
        }

        auto& inner = std::get<ir::ProjectionRelation>(rel.source->node);

        RawScalarCostEstimator inner_estimator;
        for (auto&& [id, scalar] : inner.projectors) {
            inner_estimator.estimateProjector(id, *scalar);
        }
        RawScalarCostEstimator outer_estimator;
        for (auto&& [id, scalar] : rel.projectors) {
            outer_estimator.estimateProjector(id, *scalar);
        }

        ScalarCollapseCostEstimator collapsed_estimator;
        collapsed_estimator.inner = &inner_estimator;
        for (auto&& [id, scalar] : rel.projectors) {
            collapsed_estimator.estimateProjector(id, *scalar);
        }

        int total_current_cost = inner_estimator.totalCost() + outer_estimator.totalCost() +
                                 config::Optimizer::ProjectionCostOverhead;

        if (collapsed_estimator.totalCost() > total_current_cost) {
            llog::trace(
                "projection collapse skipped, cost too high: collapsed={}, old={}",
                collapsed_estimator.totalCost(),
                total_current_cost);
            return std::move(self);
        }

        ctx.setChanges().note(
            "projection collapsed, cost: collapsed={}, old={}",
            collapsed_estimator.totalCost(),
            total_current_cost);
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

        self.node = ir::ProjectionRelation{
            .source = std::move(inner.source),
            .projectors = std::move(collapsed),
        };

        return std::move(self);
    }

    auto construct(auto& node, auto& self) {
        auto pass_name = "projection_collapse";
        auto name = rfl::type_name_t<decltype(node)>().name();

        if constexpr (requires { optimize(node, self); }) {
            llog::trace("applying {} step for {}", pass_name, name);
            return optimize(node, self);
        } else {
            return std::move(self);
        }
    }
};

}  // namespace

ir::Program projectionCollapse(ir::Program program, Context& ctx) {
    ir::Program result{
        .statements = {},
        .field_binding = program.field_binding,
    };

    Optimizer opt{.ctx = ctx};
    for (auto&& statement : program.statements) {
        result.statements.push_back(opt.pass(std::move(statement)));
    }

    return result;
}

}  // namespace lsql::opt

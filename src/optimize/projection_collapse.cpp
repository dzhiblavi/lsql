#include "optimize/projection_collapse.h"

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"
#include "ir/Scalars.h"

#include "config/build_settings.h"
#include "util/TreeWalker.h"
#include "util/logging.h"

#include <rfl.hpp>

namespace lsql::opt {

namespace {

struct ScalarCostEstimator : util::TreeWalker<ScalarCostEstimator> {
    using Optimizer = config::Optimizer;
    using Base = util::TreeWalker<ScalarCostEstimator>;
    using Base::visit;

    virtual ~ScalarCostEstimator() = default;
    virtual int view(const ir::FieldScalar&) = 0;

    void visit(const ir::Scalar& s) {
        Base::visit(s);
        auto cost = std::visit([&](const auto& c) { return view(c); }, s.node);
        setScalarCost(&s, cost);
    }

    int cost(FieldId id) const {
        auto it = cost_.find(id);
        verify(it != cost_.end());
        return it->second;
    }

    int view(const ir::ValueScalar&) { return 0; }

    int view(const ir::CoalesceScalar& s) {
        int cost = Optimizer::CoalesceCostOverhead;
        for (auto&& child : s.args) {
            cost += scalarCost(&child);
        }
        return cost;
    }

    int view(const ir::FnCallScalar& s) {
        int cost = 0;
        for (auto&& child : s.args) {
            cost += scalarCost(&child);
        }

        util::match(
            s.function,
            util::Overloaded{
                [&](const func::Lower&) { cost += Optimizer::LowerCostOverhead; },
                [&](const func::SplitPart&) { cost += Optimizer::SplitPartCostOverhead; },
                [&](const func::Substr&) { cost += Optimizer::SubstrCostOverhead; },
                [&](const func::ParseTimestamp&) { cost += Optimizer::ParseTimestampCostOverhead; },
                [&](const func::JSONValue&) { cost += Optimizer::JSONValueCostOverhead; },
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
                [&](const func::Less&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Greater&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::LessEqual&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::GreaterEqual&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::And&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Or&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Divide&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Add&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Subtract&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::Percentile&) { cost += Optimizer::BinaryOpCostOverhead; },
                [&](const func::CountNonNull&) {},
                [&](const func::CountAll&) {},
                [&](const func::Min&) {},
                [&](const func::Max&) {},
                [&](const func::Sum&) {},
            });

        return cost;
    }

    void estimateProjector(FieldId id, const ir::Scalar& s) {
        visit(s);
        auto cost = scalarCost(&s);
        cost_[id] = cost;
        total_cost_ += cost;
    }

    int totalCost() const { return total_cost_; }

 protected:
    int setScalarCost(const ir::Scalar* s, int cost) {
        verify(!scalar_cost_.contains(s));
        return scalar_cost_[s] = cost;
    }

    int scalarCost(const ir::Scalar* s) const {
        verify(scalar_cost_.contains(s));
        return scalar_cost_.at(s);
    }

    int total_cost_ = 0;
    std::unordered_map<const ir::Scalar*, int> scalar_cost_;
    std::unordered_map<FieldId, int> cost_;
};

struct RawScalarCostEstimator : ScalarCostEstimator {
    int view(const ir::FieldScalar&) override { return 0; }
};

struct ScalarCollapseCostEstimator : ScalarCostEstimator {
    const ScalarCostEstimator* inner = nullptr;
    int view(const ir::FieldScalar& outer) override { return inner->cost(outer.field_id); }
};

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
            return;
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

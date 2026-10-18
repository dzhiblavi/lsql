#pragma once

#include "config/optimizer_settings.h"
#include "ir/Scalars.h"
#include "util/TreeWalker.h"
#include "util/deep_copy.h"

#include <unordered_map>
#include <unordered_set>

namespace lsql::opt {

struct ScalarCostEstimator : util::TreeWalker<ScalarCostEstimator> {
    using enum config::OptimizerSetting;
    using Base = util::TreeWalker<ScalarCostEstimator>;
    using Base::visit;

    int64_t view(const ir::FieldScalar&) { return 0; }

    void visit(const ir::Scalar& s) {
        if (auto* cached = std::get_if<ir::CachedScalar>(&s.node)) {
            if (cached_.contains(*cached->scalar)) {
                setScalarCost(&s, optimizer.get(CachedScalarCostOverhead));
                return;
            }
            cached_.insert(util::deepCopy(*cached->scalar));
        }

        Base::visit(s);
        auto cost = std::visit([&](const auto& c) { return view(c); }, s.node);
        setScalarCost(&s, cost);
    }

    int64_t cost(const ir::Scalar& scalar) const { return scalarCost(&scalar); }

    int64_t view(const ir::ValueScalar&) { return 0; }

    int64_t view(const ir::CachedScalar& s) {
        return scalarCost(s.scalar.get()) + optimizer.get(CachedScalarCostOverhead);
    }

    int64_t view(const ir::CoalesceScalar& s) {
        int64_t cost = optimizer.get(CoalesceCostOverhead);
        for (auto&& child : s.args) {
            cost += scalarCost(&child);
        }
        return cost;
    }

    int64_t view(const ir::FnCallScalar& s) {
        int64_t cost = 0;
        for (auto&& child : s.args) {
            cost += scalarCost(&child);
        }

        util::match(
            s.function,
            util::Overloaded{
                [&](const func::Lower&) { cost += optimizer.get(LowerCostOverhead); },
                [&](const func::SplitPart&) { cost += optimizer.get(SplitPartCostOverhead); },
                [&](const func::Substr&) { cost += optimizer.get(SubstrCostOverhead); },
                [&](const func::ParseTimestamp&) {
                    cost += optimizer.get(ParseTimestampCostOverhead);
                },
                [&](const func::JSONValue&) { cost += optimizer.get(JSONValueCostOverhead); },
                [&](const func::Coalesce&) { cost += optimizer.get(CoalesceCostOverhead); },
                [&](const func::Cast& f) {
                    verify_dbg(s.args.size() == 1);

                    if (f.cast_to == ValueType::String) {
                        cost += optimizer.get(CastToStringCostOverhead);
                    }
                    if (s.args.front().value_type == ValueType::String) {
                        cost += optimizer.get(ParseStringCostOverhead);
                    }
                },
                [&](const func::Like&) { cost += optimizer.get(RegexCostOverhead); },
                [&](const func::RSubstr&) { cost += optimizer.get(RegexCostOverhead); },
                [&](const func::BooleanNegate&) { cost += optimizer.get(UnaryOpCostOverhead); },
                [&](const func::Equal&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::NotEqual&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Less&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Greater&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::LessEqual&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::GreaterEqual&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::And&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Or&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Divide&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Add&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Subtract&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::Percentile&) { cost += optimizer.get(BinaryOpCostOverhead); },
                [&](const func::CountNonNull&) {},
                [&](const func::CountAll&) {},
                [&](const func::Min&) {},
                [&](const func::Max&) {},
                [&](const func::Sum&) {},
            });

        return cost;
    }

    int64_t estimate(const ir::Scalar& s) {
        visit(s);
        auto result = scalarCost(&s);
        total_cost_ += result;
        return result;
    }

    int64_t totalCost() const { return total_cost_; }

 private:
    int64_t setScalarCost(const ir::Scalar* s, int64_t cost) {
        verify(!scalar_cost_.contains(s));
        return scalar_cost_[s] = cost;
    }

    int64_t scalarCost(const ir::Scalar* s) const {
        verify(scalar_cost_.contains(s));
        return scalar_cost_.at(s);
    }

    config::OptimizerSettings& optimizer = config::optimizer();
    int64_t total_cost_ = 0;
    std::unordered_map<const ir::Scalar*, int64_t> scalar_cost_;
    std::unordered_set<ir::Scalar> cached_;
};

}  // namespace lsql::opt

#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class Coalesce : public Expression, public std::enable_shared_from_this<Coalesce> {
    struct Aggr : Aggregator {
        explicit Aggr(std::vector<AggregatorPtr> aggregators) : aggregators(aggregators) {}

        void feed(const exec::Record& rec) override {
            for (auto&& aggregator : aggregators) {
                aggregator->feed(rec);
            }
        }

        Value get() override {
            for (auto&& aggregator : aggregators) {
                if (auto value = aggregator->get(); value != null) {
                    return value;
                }
            }

            return null;
        }

        std::vector<AggregatorPtr> aggregators;
    };

 public:
    explicit Coalesce(std::vector<ExpressionPtr> values) : values_(std::move(values)) {}

    ValueType valueType() const override { return values_.front()->valueType(); }

    AggregatorPtr aggregator() const override {
        std::vector<AggregatorPtr> aggregators;
        aggregators.reserve(values_.size());
        for (auto&& value : values_) {
            aggregators.push_back(value->aggregator());
        }
        return std::make_shared<Aggr>(std::move(aggregators));
    }

    Value eval(const exec::Record& record) const override {
        for (auto&& expr : values_) {
            if (auto value = expr->eval(record); value != null) {
                return value;
            }
        }
        return null;
    }

    Value eval(const std::vector<exec::ConstRecordPtr>& group) const override {
        for (auto&& expr : values_) {
            if (auto value = expr->eval(group); value != null) {
                return value;
            }
        }
        return null;
    }

 private:
    std::vector<ExpressionPtr> values_;
};

}  // namespace lsql::exec

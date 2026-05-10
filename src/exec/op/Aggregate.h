#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/Projection.h"
#include "exec/op/Source.h"

#include <cassert>
#include <vector>

namespace lsql::exec {

class Aggregate : public Source, public Record, public std::enable_shared_from_this<Aggregate> {
 public:
    Aggregate(OperationPtr source, ProjectionList projectors)
        : Source(1, source->minPhase())
        , source_(std::move(source))
        , projectors_(std::move(projectors)) {}

 private:
    void push(int phase) override {
        if (first_phase_ == -1) {
            return;
        }

        if (phase <= first_phase_) {
            // consume()
            return;
        }

        pushValue(phase);
    }

    bool pushValue(int phase) {
        assert(phase >= first_phase_);

        if (active(phase) && emit(phase, this)) {
            emit(phase, nullptr);
        }

        return false;
    }

    // Subscriber
    bool consume(int phase, const exec::Record* record) {
        assert(phase == first_phase_);

        if (aggregators_.size() != projectors_.size()) {
            assert(aggregators_.empty());
            aggregators_.reserve(projectors_.size());
            for (auto&& proj : projectors_) {
                aggregators_.push_back(proj->expr->aggregator());
            }
        }

        assert(aggregators_.size() == projectors_.size());

        if (record != nullptr) {
            for (auto&& aggregator : aggregators_) {
                aggregator->feed(*record);
            }

            return active(phase);
        }

        // end of stream
        values_.reserve(aggregators_.size());
        for (size_t i = 0; i < projectors_.size(); ++i) {
            values_.emplace(projectors_[i]->name, aggregators_[i]->get());
        }
        aggregators_.clear();

        return pushValue(phase);
    }

    // Operation
    void init(int out_phase) override {
        if (first_phase_ != -1) {
            // this may be an incorrect expectation
            assert(out_phase >= first_phase_);
            return;
        }

        first_phase_ = out_phase;
        source_->subscribe(out_phase, &sub_);
    }

    // Record
    values_t values() const override {
        values_t values;
        for (auto&& [k, v] : values_) {
            values.emplace(k, v);
        }
        return values;
    }

    // Record
    Value value(std::string_view name) const override {
        auto it = values_.find(name);
        return it == values_.end() ? null : it->second;
    }

    // Record
    exec::ConstRecordPtr clone() const override { return shared_from_this(); }

    OperationPtr source_;
    ProjectionList projectors_;
    MemberSubscriber<Aggregate> sub_{this, &Aggregate::consume};

    // phase state
    int first_phase_ = -1;
    std::vector<exec::AggregatorPtr> aggregators_;
    std::unordered_map<std::string_view, Value> values_;
};

SourcePtr aggregate(OperationPtr source, ProjectionList slist) {
    return std::make_shared<Aggregate>(std::move(source), std::move(slist));
}

}  // namespace lsql::exec

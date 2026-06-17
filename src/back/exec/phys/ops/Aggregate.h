#pragma once

#include "back/exec/expr/Aggregate.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"
#include "back/exec/phys/Source.h"

namespace lsql::back::exec::phys {

struct AggregateState : Record {
    // Record
    const Value& value(SlotId slot) const override {
        verify_dbg(0 <= slot && slot < values.size());
        return values[slot];
    }

    // Record
    ConstRecordPtr cloneImpl() const override { return shared_from_this(); }
    std::vector<Value> values;
};

class AggregateCollector : public OperationBase<AggregateCollector> {
 public:
    AggregateCollector(
        int id, const std::vector<Arc<exec::Aggregate>>& aggregates, Arc<AggregateState> state)
        : OperationBase(id)
        , state_(std::move(state)) {
        aggregators_.reserve(aggregates.size());
        for (auto&& a : aggregates) {
            aggregators_.push_back(a->aggregator());
        }

        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record != nullptr) {
            for (auto&& aggregator : aggregators_) {
                aggregator->feed(*record);
            }
            return active();
        }

        // end of stream
        state_->values.reserve(aggregators_.size());
        for (auto&& aggregator : aggregators_) {
            state_->values.push_back(aggregator->get());
        }
        aggregators_.clear();

        if (emit(state_.get())) {
            emit(nullptr);
        }
        return false;
    }

    std::vector<Arc<Aggregator>> aggregators_;
    Arc<AggregateState> state_;

    MemberSubscriber<AggregateCollector> sub_{
        this,
        &AggregateCollector::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

class AggregateEmitter : public Source, public OperationBase<AggregateEmitter> {
 public:
    AggregateEmitter(int id, Arc<AggregateState> state)
        : OperationBase(id)
        , state_(std::move(state)) {}

    void push() override { pushValue(); }

 private:
    bool pushValue() {
        if (active() && emit(state_.get())) {
            emit(nullptr);
        }

        return false;
    }

    Arc<AggregateState> state_;
};

}  // namespace lsql::back::exec::phys

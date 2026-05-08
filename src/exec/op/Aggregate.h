#pragma once

#include "exec/expr/Expression.h"
#include "exec/op/Projection.h"

#include <cassert>
#include <vector>

namespace lsql::exec {

class AggregateRecord : public exec::Record {
 public:
    AggregateRecord(std::shared_ptr<const ProjectionList> projectors, std::vector<Value> values)
        : projectors_(projectors)
        , values_(std::move(values)) {}

    values_t values() const override {
        values_t values;
        for (size_t i = 0; i < projectors_->size(); ++i) {
            values.emplace((*projectors_)[i]->name, values_[i]);
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = std::ranges::find(*projectors_, name, [](auto&& i) { return i->name; });
        assert(it != projectors_->end());
        return values_[it - projectors_->begin()];
    }

    exec::ConstRecordPtr clone() const override { return std::make_shared<AggregateRecord>(*this); }

 private:
    std::shared_ptr<const ProjectionList> projectors_;
    std::vector<Value> values_;
};

class Aggregate : public Operation, public std::enable_shared_from_this<Aggregate> {
 public:
    Aggregate(OperationPtr source, ProjectionList projectors)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , projectors_(std::move(projectors)) {}

 private:
    bool consume(int phase, const exec::Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;

            assert(aggregators_.empty());
            aggregators_.reserve(projectors_.size());
            for (auto&& proj : projectors_) {
                aggregators_.push_back(proj->expr->aggregator());
            }
        }

        assert(aggregators_.size() == projectors_.size());

        if (record != nullptr) {
            for (size_t i = 0; i < projectors_.size(); ++i) {
                aggregators_[i]->feed(*record);
            }

            return active(phase);
        }

        // end of stream
        std::vector<Value> values;
        values.reserve(aggregators_.size());
        for (auto&& aggregator : aggregators_) {
            values.push_back(aggregator->get());
        }
        aggregators_.clear();

        AggregateRecord rec({shared_from_this(), &projectors_}, std::move(values));
        emit(phase, &rec);
        return emit(phase, nullptr);
    }

    void subscribe(int phase) override { source_->subscribe(phase, &sub_); }

    OperationPtr source_;
    ProjectionList projectors_;
    MemberSubscriber<Aggregate> sub_{this, &Aggregate::consume};

    // phase state
    int curr_phase_ = -1;
    std::vector<exec::AggregatorPtr> aggregators_;
};

OperationPtr aggregate(OperationPtr source, ProjectionList slist) {
    return std::make_shared<Aggregate>(std::move(source), std::move(slist));
}

}  // namespace lsql::exec

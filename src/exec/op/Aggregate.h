#pragma once

#include "core/verify.h"
#include "exec/expr/Expression.h"
#include "exec/op/Profiler.h"
#include "exec/op/Projection.h"
#include "exec/op/Source.h"

#include <vector>

namespace lsql::exec {

class Aggregate : public Source, public Record, public std::enable_shared_from_this<Aggregate> {
 public:
    Aggregate(OperationPtr source, ProjectionList projectors)
        : Source(source->minPhase(), Profiler::profiler().registerOperation(this, "Aggregate"))
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
        verify(phase >= first_phase_);

        if (active(phase) && emit(phase, this)) {
            emit(phase, nullptr);
        }

        return false;
    }

    // Subscriber
    bool consume(int phase, const exec::Record* record) {
        verify(phase == first_phase_);

        if (aggregators_.size() != projectors_.size()) {
            verify(aggregators_.empty());
            aggregators_.reserve(projectors_.size());
            for (auto&& proj : projectors_) {
                aggregators_.push_back(proj->expr->aggregator());
            }
        }

        verify(aggregators_.size() == projectors_.size());

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
            verify(out_phase >= first_phase_);
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

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (ctx.phase < first_phase_) {
            // we return nothing on this phase
            verify(source.empty());
            return {};
        }

        if (ctx.phase == first_phase_) {
            auto item =
                ExplanationItem()
                    .line(
                        "Aggregate w/store ({} projections) [id={}]", projectors_.size(), uniq_id_)
                    .child(source);

            if (hasSubscriber(ctx.phase, ctx.requester)) {
                return item;
            } else {
                ctx.explanation.insert(item, this);
                return {};
            }
        }

        // phase > first_phase_
        if (hasSubscriber(ctx.phase, ctx.requester)) {
            return ExplanationItem().line("Aggregate stored [id={}]", uniq_id_);
        }

        return {};
    }

    OperationPtr source_;
    ProjectionList projectors_;

    MemberSubscriber<Aggregate> sub_{
        this,
        &Aggregate::consume,
        handle_.inputHandle(&sub_),
    };

    // phase state
    int first_phase_ = -1;
    std::vector<exec::AggregatorPtr> aggregators_;
    std::unordered_map<std::string_view, Value> values_;
};

SourcePtr aggregate(OperationPtr source, ProjectionList slist) {
    return std::make_shared<Aggregate>(std::move(source), std::move(slist));
}

}  // namespace lsql::exec

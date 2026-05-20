#pragma once

#include "core/verify.h"
#include "exec/expr/Expression.h"
#include "exec/op/Projection.h"
#include "exec/op/Source.h"

#include <vector>

namespace lsql::exec {

class Aggregate : public Source, public OperationBase<Aggregate>, public Record {
 public:
    Aggregate(OperationPtr source, ProjectionList projectors, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
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
    bool consume(int phase, const Record* record) {
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
            values_.emplace(projectors_[i]->field_id, aggregators_[i]->get());
        }
        aggregators_.clear();

        return pushValue(phase);
    }

    // Operation
    void init(int out_phase, const RequiredFields& downstream) override {
        if (first_phase_ == -1) {
            first_phase_ = out_phase;
        } else {
            // this may be an incorrect expectation
            verify(out_phase >= first_phase_);
        }

        // resubscribe even if already subscribed
        // idempotent but will update required fields if needed
        source_->subscribe(first_phase_, &sub_, getRequiredFields(downstream));
    }

    RequiredFields getRequiredFields(const RequiredFields& downstream) const {
        RequiredFields result = RequiredFields::withNone();

        for (auto&& proj : projectors_) {
            if (!downstream.requiresField(proj->field_id)) {
                continue;
            }

            result.merge(proj->expr->requiredFields());
        }

        return result;
    }

    // Record
    ids_t ids() const override {
        ids_t ids;
        for (auto&& [id, _] : values_) {
            ids.insert(id);
        }
        return ids;
    }

    // Record
    Value value(FieldId id) const override {
        auto it = values_.find(id);
        return it == values_.end() ? null : it->second;
    }

    // Record
    ConstRecordPtr cloneImpl() const override { return shared_from_this(); }

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
                    .line("{} (store, {} projections)", description(ctx.phase), projectors_.size())
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
            return ExplanationItem().line("{} (push stored)", description(ctx.phase));
        }

        return {};
    }

    OperationPtr source_;
    ProjectionList projectors_;

    MemberSubscriber<Aggregate> sub_{
        this,
        &Aggregate::consume,
        prof_.inputHandle(&sub_),
    };

    // phase state
    int first_phase_ = -1;
    std::vector<AggregatorPtr> aggregators_;
    std::unordered_map<FieldId, Value> values_;
};

SourcePtr aggregate(OperationPtr source, ProjectionList slist, ConstFieldBindingPtr binding) {
    return std::make_shared<Aggregate>(std::move(source), std::move(slist), std::move(binding));
}

}  // namespace lsql::exec

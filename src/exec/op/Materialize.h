#pragma once

#include "core/verify.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"

namespace lsql::exec {

class Materialize : public Source, public OperationBase<Materialize> {
 public:
    Materialize(OperationPtr source, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source)) {
        prof::addEdge(&prof_sub_, &prof_);
    }

    void push(int phase) override {
        if (first_phase_ == -1) {
            return;
        }

        if (phase <= first_phase_) {
            // consume()
            return;
        }

        pushMaterialized(phase);
    }

 private:
    // Subscriber
    bool consume(int phase, const Record* record) {
        verify(phase == first_phase_);

        if (!materialized_) {
            materialized_.emplace();
        }

        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        materialized_->push_back(record->clone());
        return emit(phase, record);
    }

    void pushMaterialized(int phase) {
        verify(materialized_.has_value());

        if (!active(phase)) {
            return;
        }

        for (auto&& record : *materialized_) {
            if (!emit(phase, record.get())) {
                return;
            }
        }

        emit(phase, nullptr);
    }

    // Operation
    void init(int out_phase, const FieldSet& downstream) override {
        if (first_phase_ == -1) {
            first_phase_ = out_phase;
        } else {
            // this may be an incorrect expectation
            verify(out_phase >= first_phase_);
        }

        source_->subscribe(first_phase_, &sub_, downstream);
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (ctx.phase < first_phase_) {
            // we do nothing on this phase
            verify(source.empty());
            return {};
        }

        if (ctx.phase == first_phase_) {
            auto item = ExplanationItem()
                            .line("{} store passthrough", description(ctx.phase))
                            .child(source);

            if (hasSubscriber(ctx.phase, ctx.requester)) {
                return item;
            } else {
                ctx.explanation.insert(item, this);
                return {};
            }
        }

        // phase > first_phase_
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} scan stored", description(ctx.phase));
    }

    OperationPtr source_;

    prof::ScopeHandle<ScopeMetrics> prof_sub_ = prof::newScope<ScopeMetrics>("{} input", name());

    MemberSubscriber<Materialize> sub_{
        this,
        &Materialize::consume,
        &prof_sub_,
    };

    std::optional<std::vector<ConstRecordPtr>> materialized_ = std::nullopt;
    int first_phase_ = -1;
};

SourcePtr materialize(OperationPtr source, ConstFieldBindingPtr binding) {
    return std::make_shared<Materialize>(std::move(source), std::move(binding));
}

}  // namespace lsql::exec

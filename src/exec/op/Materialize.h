#pragma once

#include "core/verify.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"

namespace lsql::exec {

class Materialize : public Source, public OperationBase<Materialize> {
 public:
    Materialize(OperationPtr source)
        : OperationBase(source->minPhase(), "Materialize")
        , source_(std::move(source)) {}

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
    void init(int out_phase) override {
        if (first_phase_ != -1) {
            // this may be an incorrect expectation
            verify(out_phase >= first_phase_);
            return;
        }

        first_phase_ = out_phase;
        source_->subscribe(first_phase_, &sub_);
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
            auto item = ExplanationItem().line("{} store passthrough", name()).child(source);

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

        return ExplanationItem().line("{} scan stored", name());
    }

    OperationPtr source_;
    MemberSubscriber<Materialize> sub_{
        this,
        &Materialize::consume,
        prof_.inputHandle(&sub_),
    };
    std::optional<std::vector<ConstRecordPtr>> materialized_ = std::nullopt;
    int first_phase_ = -1;
};

SourcePtr materialize(OperationPtr source) {
    return std::make_shared<Materialize>(std::move(source));
}

}  // namespace lsql::exec

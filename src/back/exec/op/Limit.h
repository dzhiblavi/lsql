#pragma once

#include "back/exec/op/MemberSubscriber.h"
#include "back/exec/op/OperationBase.h"

namespace lsql::back::exec {

class Limit : public OperationBase<Limit> {
 public:
    Limit(OperationPtr source, int limit, ConstFieldBindingPtr binding)
        : OperationBase(source->minPhase(), std::move(binding))
        , source_(std::move(source))
        , limit_(limit) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

 private:
    // Subscriber
    bool consume(int phase, const Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            curr_limit_ = limit_;
        }

        if (record == nullptr) {
            return emit(phase, nullptr);
        }

        if (curr_limit_ == 0) {
            return false;
        }

        --curr_limit_;
        if (!emit(phase, record)) {
            return false;
        }

        if (curr_limit_ == 0) {
            return emit(phase, nullptr);
        }

        return active(phase);
    }

    // Operation
    void init(int phase, const FieldSet& downstream) override {
        source_->subscribe(phase, &sub_, downstream);
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem()
            .line("{} [limit={}]", description(ctx.phase), limit_)
            .child(source);
    }

    OperationPtr source_;
    const int limit_;

    MemberSubscriber<Limit> sub_{
        this,
        &Limit::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };

    // phase state
    int curr_phase_ = 0;
    int curr_limit_ = limit_;
};

OperationPtr limit(OperationPtr source, int limit, ConstFieldBindingPtr binding) {
    return std::make_shared<Limit>(std::move(source), limit, std::move(binding));
}

}  // namespace lsql::back::exec

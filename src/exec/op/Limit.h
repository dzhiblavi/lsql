#pragma once

#include "core/verify.h"
#include "exec/op/Operation.h"

namespace lsql::exec {

class Limit : public Operation {
 public:
    Limit(OperationPtr source, int limit)
        : Operation(source->minPhase(), "Limit")
        , source_(std::move(source))
        , limit_(limit) {}

 private:
    // Subscriber
    bool consume(int phase, const exec::Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            curr_limit_ = limit_;
        }

        verify(curr_limit_ > 0);
        if (!emit(phase, record)) {
            return false;
        }

        if (--curr_limit_ == 0) {
            return emit(phase, nullptr);
        } else {
            return active(phase);
        }
    }

    // Operation
    void init(int phase) override { source_->subscribe(phase, &sub_); }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto source = source_->explain(ctx.withRequester(&sub_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("Limit {}", limit_).child(source);
    }

    OperationPtr source_;
    const int limit_;
    MemberSubscriber<Limit> sub_{
        this,
        &Limit::consume,
        prof_.inputHandle(&sub_),
    };

    // phase state
    int curr_phase_ = 0;
    int curr_limit_ = limit_;
};

OperationPtr limit(OperationPtr source, int limit) {
    return std::make_shared<Limit>(std::move(source), limit);
}

}  // namespace lsql::exec

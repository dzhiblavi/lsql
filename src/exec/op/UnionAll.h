#pragma once

#include "core/verify.h"
#include "exec/op/MemberSubscriber.h"
#include "exec/op/Operation.h"

namespace lsql::exec {

class UnionAll : public Operation {
    enum class State {
        RunningBoth,  // both subscriptions active
        RunningOne,   // only one subscription active (done_[I] == 0)
    };

 public:
    UnionAll(OperationPtr l, OperationPtr r)
        : Operation(std::max(l->minPhase(), r->minPhase()), "UnionAll")
        , l_(std::move(l))
        , r_(std::move(r)) {}

 private:
    template <int Index>
    bool consume(int phase, const Record* record) {
        std::lock_guard lg(m_emit_);

        if (done_[Index]) {
            // the stream has ended prematurely via receiver request
            verify(done_[1 - Index]);
            reset();
            return false;
        }

        if (record == nullptr) {
            if (finished<Index>()) {
                // the second subscription finished. no more records
                reset();
                verify(!emit(phase, nullptr));
            }

            // we want to cancel current subscription anyhow
            // this will cancel l OR r only, the second one remains untouched
            return false;
        }

        if (!emit(phase, record)) {
            // the receiver does not want more records
            // so we set both done_ to true to cancel both subscriptions
            done_[0] = done_[1] = true;
            return false;
        }

        return true;
    }

    template <int Index>
    bool finished() {
        verify(!done_[Index]);
        done_[Index] = true;
        return done_[1 - Index];
    }

    void reset() { done_[0] = done_[1] = false; }

    // Operation
    void init(int phase) override {
        l_->subscribe(phase, &sub_l_);
        r_->subscribe(phase, &sub_r_);
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        auto l = l_->explain(ctx.withRequester(&sub_l_));
        auto r = r_->explain(ctx.withRequester(&sub_r_));

        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line(fullName()).child(l).child(r);
    }

    OperationPtr l_;
    OperationPtr r_;

    std::mutex m_emit_;
    bool done_[2] = {false};
    MemberSubscriber<UnionAll> sub_l_{this, &UnionAll::consume<0>, prof_.inputHandle(&sub_l_)};
    MemberSubscriber<UnionAll> sub_r_{this, &UnionAll::consume<1>, prof_.inputHandle(&sub_r_)};
};

OperationPtr unionAll(OperationPtr l, OperationPtr r) {
    return std::make_shared<UnionAll>(std::move(l), std::move(r));
}

}  // namespace lsql::exec

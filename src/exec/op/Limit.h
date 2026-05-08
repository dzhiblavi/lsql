#pragma once

#include "exec/op/Operation.h"

namespace lsql::exec {

class Limit : public Operation {
 public:
    Limit(OperationPtr source, int limit)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , limit_(limit) {}

 private:
    bool consume(int phase, const exec::Record* record) {
        if (curr_phase_ != phase) {
            curr_phase_ = phase;
            curr_limit_ = limit_;
        }

        assert(curr_limit_ > 0);
        if (!emit(phase, record)) {
            return false;
        }

        if (--curr_limit_ == 0) {
            return emit(phase, nullptr);
        } else {
            return active(phase);
        }
    }

    void subscribe(int phase) override { source_->subscribe(phase, &sub_); }

    OperationPtr source_;
    const int limit_;
    MemberSubscriber<Limit> sub_{this, &Limit::consume};

    // phase state
    int curr_phase_ = 0;
    int curr_limit_ = limit_;
};

OperationPtr limit(OperationPtr source, int limit) {
    return std::make_shared<Limit>(std::move(source), limit);
}

}  // namespace lsql::exec

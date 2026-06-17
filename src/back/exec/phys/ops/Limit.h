#pragma once

#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

namespace lsql::back::exec::phys {

class Limit : public OperationBase<Limit> {
 public:
    Limit(int id, int limit) : OperationBase(id), curr_limit_(limit) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        if (curr_limit_ == 0) {
            return emit(nullptr);
        }

        --curr_limit_;
        if (!emit(record)) {
            return false;
        }

        if (curr_limit_ == 0) {
            return emit(nullptr);
        }

        return active();
    }

    int curr_limit_;
    MemberSubscriber<Limit> sub_{
        this,
        &Limit::consume,
        prof::newScope<ScopeMetrics>("{} input", name()),
    };
};

}  // namespace lsql::back::exec::phys

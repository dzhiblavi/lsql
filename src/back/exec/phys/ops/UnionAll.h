#pragma once

#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"
#include "util/verify.h"

namespace lsql::back::exec::phys {

class UnionAll : public OperationBase<UnionAll> {
 public:
    explicit UnionAll(int id) : OperationBase(id) {
        prof::addEdge(sub_l_.scopeHandle(), prof_);
        prof::addEdge(sub_r_.scopeHandle(), prof_);
    }

    Subscriber* subLeft() { return &sub_l_; }
    Subscriber* subRight() { return &sub_r_; }

 private:
    template <int Index>
    bool consume(const Record* record) {
        if (done_[Index]) {
            // the stream has ended prematurely via receiver request
            verify_dbg(done_[1 - Index]);
            reset();
            return false;
        }

        if (record == nullptr) {
            if (finished<Index>()) {
                // the second subscription finished. no more records
                reset();
                [[maybe_unused]] auto res = emit(nullptr);
                verify_dbg(!res);
            }

            // we want to cancel current subscription anyhow
            // this will cancel l OR r only, the second one remains untouched
            return false;
        }

        if (!emit(record)) {
            // the receiver does not want more records
            // so we set both done_ to true to cancel both subscriptions
            done_[0] = done_[1] = true;
            return false;
        }

        return true;
    }

    template <int Index>
    bool finished() {
        verify_dbg(!done_[Index]);
        done_[Index] = true;
        return done_[1 - Index];
    }

    void reset() { done_[0] = done_[1] = false; }

    bool done_[2] = {false};
    std::mutex m_;

    MemberSubscriber<UnionAll, LockMixin> sub_l_{
        this,
        &UnionAll::consume<0>,
        prof::newScope<ScopeMetrics>("{} input(L)", name()),
        &m_,
    };
    MemberSubscriber<UnionAll, LockMixin> sub_r_{
        this,
        &UnionAll::consume<1>,
        prof::newScope<ScopeMetrics>("{} input(R)", name()),
        &m_,
    };
};

}  // namespace lsql::back::exec::phys

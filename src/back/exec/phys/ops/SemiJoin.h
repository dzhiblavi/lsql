#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "util/instrument/Counters.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::back::exec::phys {

struct SemiJoinMetrics {
    void reset() { match_set_size.set(0); }
    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder("match_set_size: {}", match_set_size.value());
    }

    instr::Counter<size_t> match_set_size{0};
};

struct SemiJoinState {
    absl::flat_hash_set<Value> values;
};

class SemiJoinMatchCollector : public OperationBase<SemiJoinMatchCollector, SemiJoinMetrics> {
 public:
    SemiJoinMatchCollector(int id, Arc<SemiJoinState> state)
        : OperationBase(id)
        , state_(std::move(state)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            updateMetrics();
            return false;
        }

        state_->values.insert(record->value(0));
        return true;
    }

    void updateMetrics() {
        if (auto m = prof_.metrics()) {
            m->custom<SemiJoinMetrics>().match_set_size.set(state_->values.size());
        }
    }

    Arc<SemiJoinState> state_;
    MemberSubscriber<SemiJoinMatchCollector> sub_{
        this,
        &SemiJoinMatchCollector::consume,
        prof::newScope<ScopeMetrics>("{} match set input", name()),
    };
};

class SemiJoinMatcher : public OperationBase<SemiJoinMatcher> {
 public:
    SemiJoinMatcher(int id, Arc<SemiJoinState> state, Arc<Scalar> join_key)
        : OperationBase(id)
        , state_(std::move(state))
        , join_key_(std::move(join_key)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr || state_->values.empty()) {
            return emit(nullptr);
        }

        if (state_->values.contains(join_key_->eval(*record))) {
            if (!emit(record)) {
                return false;
            }
            return true;
        }

        if (!active()) {
            return false;
        }

        return true;
    }

    Arc<SemiJoinState> state_;
    Arc<Scalar> join_key_;
    MemberSubscriber<SemiJoinMatcher> sub_{
        this,
        &SemiJoinMatcher::consume,
        prof::newScope<ScopeMetrics>("{} src input", name()),
    };
};

}  // namespace lsql::back::exec::phys

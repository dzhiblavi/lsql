#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "util/instrument/Counters.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::back::exec::phys {

struct HashSemiJoinMetrics {
    void reset() { match_set_size.set(0); }
    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder("match_set_size: {}", match_set_size.value());
    }

    instr::Counter<size_t> match_set_size{0};
};

struct HashSemiJoinState {
    absl::flat_hash_set<Value> values;
};

class HashSemiJoinBuild : public OperationBase<HashSemiJoinBuild, HashSemiJoinMetrics> {
 public:
    HashSemiJoinBuild(int id, Arc<HashSemiJoinState> state)
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
            m->custom<HashSemiJoinMetrics>().match_set_size.set(state_->values.size());
        }
    }

    Arc<HashSemiJoinState> state_;
    MemberSubscriber<HashSemiJoinBuild> sub_{
        this,
        &HashSemiJoinBuild::consume,
        prof::newScope<ScopeMetrics>("{} match set input", name()),
    };
};

class HashSemiJoinProbe : public OperationBase<HashSemiJoinProbe> {
 public:
    HashSemiJoinProbe(int id, Arc<HashSemiJoinState> state, Arc<Scalar> join_key)
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

    Arc<HashSemiJoinState> state_;
    Arc<Scalar> join_key_;
    MemberSubscriber<HashSemiJoinProbe> sub_{
        this,
        &HashSemiJoinProbe::consume,
        prof::newScope<ScopeMetrics>("{} src input", name()),
    };
};

}  // namespace lsql::back::exec::phys

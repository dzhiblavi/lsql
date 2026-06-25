#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "util/instrument/Counters.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::back::exec::phys {

struct MarkJoinMetrics {
    void reset() { match_set_size.set(0); }

    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder("match_set_size: {}", match_set_size.value());
    }

    instr::Counter<size_t> match_set_size{0};
};

class MarkJoinRecord : public Record {
 public:
    MarkJoinRecord(Value value, SlotId slot, RecordRef child)
        : child_(std::move(child))
        , value_(std::move(value))
        , value_slot_(slot) {}

    const Value& value(SlotId slot) const override {
        return slot == value_slot_ ? value_ : get(child_)->value(slot);
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<MarkJoinRecord>(value_, value_slot_, pin(child_));
    }

    RecordRef child_;
    Value value_;
    SlotId value_slot_;
};

struct MarkJoinState {
    absl::flat_hash_set<Value> values;
};

class MarkJoinMatchCollector : public OperationBase<MarkJoinMatchCollector, MarkJoinMetrics> {
 public:
    MarkJoinMatchCollector(int id, Arc<MarkJoinState> state)
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
            m->custom<MarkJoinMetrics>().match_set_size.set(state_->values.size());
        }
    }

    Arc<MarkJoinState> state_;
    MemberSubscriber<MarkJoinMatchCollector> sub_{
        this,
        &MarkJoinMatchCollector::consume,
        prof::newScope<ScopeMetrics>("{} match set input", name()),
    };
};

class MarkJoinMatcher : public OperationBase<MarkJoinMatcher> {
 public:
    MarkJoinMatcher(int id, Arc<MarkJoinState> state, Arc<Scalar> join_key, SlotId output_slot_id)
        : OperationBase(id)
        , state_(std::move(state))
        , join_key_(std::move(join_key))
        , output_slot_id_(output_slot_id) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        bool value = state_->values.contains(join_key_->eval(*record));
        MarkJoinRecord marked_record(value, output_slot_id_, record);

        if (!emit(&marked_record)) {
            return false;
        }

        return true;
    }

    Arc<MarkJoinState> state_;
    Arc<Scalar> join_key_;
    SlotId output_slot_id_;

    MemberSubscriber<MarkJoinMatcher> sub_{
        this,
        &MarkJoinMatcher::consume,
        prof::newScope<ScopeMetrics>("{} src input", name()),
    };
};

}  // namespace lsql::back::exec::phys

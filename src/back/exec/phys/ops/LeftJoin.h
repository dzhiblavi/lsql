#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/phys/MemberSubscriber.h"
#include "back/exec/phys/Operation.h"

#include "util/instrument/Counters.h"

#include <absl/container/flat_hash_map.h>

namespace lsql::back::exec::phys {

struct LeftJoinMetrics {
    void reset() {
        distinct_keys.set(0);
        stored_rows.set(0);
    }

    util::StrBuilder report() const { return shortReport(); }

    util::StrBuilder shortReport() const {
        return util::StrBuilder(
            "distinct_keys: {}, stored_rows: {}", distinct_keys.value(), stored_rows.value());
    }

    instr::Counter<size_t> distinct_keys{0};
    instr::Counter<size_t> stored_rows{0};
};

class LeftJoinRecord : public Record {
 public:
    LeftJoinRecord(RecordRef left, ConstRecordPtr right, SlotId left_columns)
        : left_(std::move(left))
        , right_(std::move(right))
        , left_columns_(left_columns) {}

    const Value& value(SlotId slot) const override {
        if (slot < left_columns_) {
            return get(left_)->value(slot);
        }
        return right_ ? right_->value(slot - left_columns_) : vnull;
    }

 private:
    ConstRecordPtr cloneImpl() const override {
        return arc<LeftJoinRecord>(pin(left_), right_, left_columns_);
    }

    RecordRef left_;
    ConstRecordPtr right_;
    SlotId left_columns_;
};

struct LeftJoinState {
    absl::flat_hash_map<Value, std::vector<ConstRecordPtr>> rows;
    size_t stored_rows = 0;
};

class LeftJoinCollector : public OperationBase<LeftJoinCollector, LeftJoinMetrics> {
 public:
    LeftJoinCollector(int id, Arc<LeftJoinState> state, ScalarPtr key)
        : OperationBase(id)
        , state_(std::move(state))
        , key_(std::move(key)) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            if (auto metrics = prof_.metrics()) {
                auto& custom = metrics->custom<LeftJoinMetrics>();
                custom.distinct_keys.set(state_->rows.size());
                custom.stored_rows.set(state_->stored_rows);
            }
            return false;
        }

        auto key = key_->eval(*record);
        if (key.type() != ValueType::Null) {
            state_->rows[std::move(key)].push_back(record->clone());
            ++state_->stored_rows;
        }

        return true;
    }

    Arc<LeftJoinState> state_;
    ScalarPtr key_;
    MemberSubscriber<LeftJoinCollector> sub_{
        this,
        &LeftJoinCollector::consume,
        prof::newScope<ScopeMetrics>("{} right input", name()),
    };
};

class LeftJoinMatcher : public OperationBase<LeftJoinMatcher> {
 public:
    LeftJoinMatcher(int id, Arc<LeftJoinState> state, ScalarPtr key, SlotId left_columns)
        : OperationBase(id)
        , state_(std::move(state))
        , key_(std::move(key))
        , left_columns_(left_columns) {
        prof::addEdge(sub_.scopeHandle(), prof_);
    }

    Subscriber* sub() { return &sub_; }

 private:
    bool consume(const Record* record) {
        if (record == nullptr) {
            return emit(nullptr);
        }

        auto key = key_->eval(*record);
        auto matches = state_->rows.find(key);
        if (key.type() == ValueType::Null || matches == state_->rows.end()) {
            LeftJoinRecord joined(record, nullptr, left_columns_);
            return emit(&joined);
        }

        for (const auto& right : matches->second) {
            LeftJoinRecord joined(record, right, left_columns_);

            if (!emit(&joined)) {
                return false;
            }
        }

        return true;
    }

    Arc<LeftJoinState> state_;
    ScalarPtr key_;
    SlotId left_columns_;
    MemberSubscriber<LeftJoinMatcher> sub_{
        this,
        &LeftJoinMatcher::consume,
        prof::newScope<ScopeMetrics>("{} left input", name()),
    };
};

}  // namespace lsql::back::exec::phys

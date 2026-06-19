#pragma once

#include "profiling/global.h"

#include "core/schema/types.h"
#include "core/types.h"
#include "core/value/Value.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::back::exec {

class Record : public std::enable_shared_from_this<Record> {
 public:
    virtual ~Record() = default;

    virtual const Value& value(SlotId slot) const = 0;

    Arc<const Record> clone() const {
        if (!weak_from_this().expired()) {
            prof::addCounter("record.share");
            return shared_from_this();
        }

        prof::addCounter("record.clone");
        return cloneImpl();
    }

 private:
    virtual Arc<const Record> cloneImpl() const = 0;
};

using RecordPtr = Arc<Record>;
using ConstRecordPtr = Arc<const Record>;
using RecordRef = std::variant<const Record*, ConstRecordPtr>;

class EmptyRecord : public Record {
 public:
    EmptyRecord() = default;

    const Value& value(SlotId) const override { return vnull; }

    static ConstRecordPtr instance() {
        static ConstRecordPtr record = arc<EmptyRecord>();
        return record;
    }

 private:
    Arc<const Record> cloneImpl() const override { return instance(); }
};

class VecRecord : public Record {
 public:
    explicit VecRecord(std::vector<Value> values) : values_(std::move(values)) {}

    const Value& value(SlotId slot) const override {
        verify_dbg(0 <= slot && slot < values_.size());
        return values_[slot];
    }

    ConstRecordPtr cloneImpl() const override { return std::make_shared<VecRecord>(*this); }
    std::span<Value> mutableValues() { return values_; }

 private:
    std::vector<Value> values_;
};

inline ConstRecordPtr pin(const RecordRef& ref) {
    return std::visit(
        util::Overloaded{
            [](const Record* record) { return record->clone(); },
            [](ConstRecordPtr record) { return record; },
        },
        ref);
}

inline const Record* get(const RecordRef& ref) {
    return std::visit(
        util::Overloaded{
            [](const Record* record) { return record; },
            [](ConstRecordPtr record) { return record.get(); },
        },
        ref);
}

}  // namespace lsql::back::exec

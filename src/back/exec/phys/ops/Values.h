#pragma once

#include "back/exec/Record.h"
#include "back/exec/phys/Operation.h"
#include "back/exec/phys/Source.h"

namespace lsql::back::exec::phys {

class ValueRecord : public Record {
 public:
    explicit ValueRecord(const Value* value) : value_(value) {}

    const Value& value([[maybe_unused]] SlotId slot) const override {
        verify_dbg(slot == 0);
        return *value_;
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<ValueRecord>(*this);
    }

    const Value* value_;
};

class Values : public Source,
               public OperationBase<Values>,
               public std::enable_shared_from_this<Values> {
 public:
    Values(int id, std::vector<Value> values) : OperationBase(id), values_(std::move(values)) {}

    void push() override {
        verify(active());

        for (const auto& value : values_) {
            ValueRecord record(&value);

            if (!emit(&record)) {
                return;
            }
        }

        emit(nullptr);
    }

 private:
    std::vector<Value> values_;
};

}  // namespace lsql::back::exec::phys

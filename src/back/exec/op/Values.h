#pragma once

#include "back/exec/Record.h"
#include "back/exec/op/OperationBase.h"
#include "back/exec/op/Source.h"

namespace lsql::back::exec {

class ValueRecord : public Record {
 public:
    explicit ValueRecord(const Value* value) : value_(std::move(value)) {}

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
    Values(std::vector<Value> values, ConstFieldBindingPtr binding)
        : OperationBase(0, std::move(binding))
        , values_(std::move(values)) {}

    void push(int phase) override {
        if (!active(phase)) {
            return;
        }

        if (requiredFields(phase).empty()) {
            auto* record = EmptyRecord::instance().get();

            for (size_t i = 0; i < values_.size(); ++i) {
                if (!emit(phase, record)) {
                    return;
                }
            }
        } else {
            for (const auto& value : values_) {
                ValueRecord record(&value);

                if (!emit(phase, &record)) {
                    return;
                }
            }
        }

        emit(phase, nullptr);
    }

 private:
    // Operation
    void init(int phase, const FieldSet& fields) override { updateFieldSet(phase, fields); }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} [count={}]", description(ctx.phase), values_.size());
    }

    std::vector<Value> values_;
};

SourcePtr values(std::vector<Value> values, ConstFieldBindingPtr binding) {
    return std::make_shared<Values>(std::move(values), std::move(binding));
}

}  // namespace lsql::back::exec

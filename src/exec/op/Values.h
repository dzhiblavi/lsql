#pragma once

#include "exec/Record.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"

namespace lsql::exec {

class ValueRecord : public Record {
 public:
    ValueRecord(FieldId id, std::shared_ptr<const Value> value)
        : id_(id)
        , value_(std::move(value)) {}

    ids_t ids() const override { return {id_}; }

    Value value(FieldId id) const override { return id == id_ ? *value_ : null; }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<ValueRecord>(*this);
    }

    FieldId id_;
    std::shared_ptr<const Value> value_;
};

class Values : public Source,
               public OperationBase<Values>,
               public std::enable_shared_from_this<Values> {
 public:
    Values(std::vector<Value> values, FieldId id, ConstFieldBindingPtr binding)
        : OperationBase(0, std::move(binding))
        , id_(id)
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
                ValueRecord record(id_, {shared_from_this(), &value});

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

    FieldId id_;
    std::vector<Value> values_;
};

SourcePtr values(std::vector<Value> values, FieldId id, ConstFieldBindingPtr binding) {
    return std::make_shared<Values>(std::move(values), id, std::move(binding));
}

}  // namespace lsql::exec

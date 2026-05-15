#pragma once

#include "exec/Record.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"

namespace lsql::exec {

class ValueRecord : public Record {
 public:
    explicit ValueRecord(std::shared_ptr<const Value> value) : value_(std::move(value)) {}

    values_t values() const override {
        values_t res;
        res["anon"] = *value_;
        return res;
    }

    Value value(std::string_view /*name*/) const override { return *value_; }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<ValueRecord>(*this);
    }

    std::shared_ptr<const Value> value_;
};

class Values : public Source,
               public OperationBase<Values>,
               public std::enable_shared_from_this<Values> {
 public:
    explicit Values(std::vector<Value> values) : OperationBase(0), values_(std::move(values)) {}

    void push(int phase) override {
        for (const auto& value : values_) {
            ValueRecord record({shared_from_this(), &value});

            if (!emit(phase, &record)) {
                return;
            }
        }

        emit(phase, nullptr);
    }

 private:
    // Operation
    void init(int /*phase*/) override {}

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} [count={}]", name(), values_.size());
    }

    std::vector<Value> values_;
};

SourcePtr values(std::vector<Value> values) {
    return std::make_shared<Values>(std::move(values));
}

}  // namespace lsql::exec

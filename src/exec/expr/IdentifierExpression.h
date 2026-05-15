#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class IdentifierExpression : public Expression {
 public:
    explicit IdentifierExpression(std::string name) : name_(std::move(name)) {}

    RequiredFields requiredFields() const override { return RequiredFields::withFields({name_}); }

    ValueType valueType() const override { return ValueType::String; }

    Value eval(const exec::Record& record) const override { return record.value(name_); }

    Value eval(const std::vector<exec::ConstRecordPtr>& group) const override {
        return eval(*group.front());
    }

    AggregatorPtr aggregator() const override {
        struct Aggr : Aggregator {
            explicit Aggr(const IdentifierExpression* self) : self(self) {}

            Value get() override { return std::move(value); }

            void feed(const exec::Record& record) override {
                if (value != null) {
                    return;
                }

                value = self->eval(record);
            }

            const IdentifierExpression* self;
            Value value;
        };

        return std::make_shared<Aggr>(this);
    }

 private:
    std::string name_;
};

}  // namespace lsql::exec

#pragma once

#include "exec/expr/Expression.h"

#include "core/Fields.h"

namespace lsql::exec {

class IdentifierExpression : public Expression {
 public:
    IdentifierExpression(FieldId id, ValueType type) : id_(id), type_(type) {}

    FieldSet requiredFields() const override { return FieldSet::withField(id_); }

    ValueType valueType() const override { return type_; }

    Value eval(const exec::Record& record) const override { return record.value(id_); }

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
    FieldId id_;
    ValueType type_;
};

}  // namespace lsql::exec

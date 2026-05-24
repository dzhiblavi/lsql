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

 private:
    FieldId id_;
    ValueType type_;
};

}  // namespace lsql::exec

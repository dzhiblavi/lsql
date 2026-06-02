#pragma once

#include "back/exec/expr/Scalar.h"

#include "core/Fields.h"

namespace lsql::back::exec {

class IdentifierScalar : public Scalar {
 public:
    IdentifierScalar(FieldId id, ValueType type) : id_(id), type_(type) {}
    FieldSet requiredFields() const override { return FieldSet::withField(id_); }
    ValueType valueType() const override { return type_; }
    Value eval(const back::exec::Record& record) const override { return record.value(id_); }

 private:
    FieldId id_;
    ValueType type_;
};

}  // namespace lsql::back::exec

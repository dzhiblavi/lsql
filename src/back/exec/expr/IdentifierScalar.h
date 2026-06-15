#pragma once

#include "back/exec/expr/Scalar.h"

namespace lsql::back::exec {

class IdentifierScalar : public Scalar {
 public:
    IdentifierScalar(SlotId slot, FieldId id, ValueType type) : slot_(slot), id_(id), type_(type) {}
    FieldSet requiredFields() const override { return FieldSet::withField(id_); }
    ValueType valueType() const override { return type_; }
    Value eval(const back::exec::Record& record) const override { return record.value(slot_); }

 private:
    SlotId slot_;
    FieldId id_;
    ValueType type_;
};

}  // namespace lsql::back::exec

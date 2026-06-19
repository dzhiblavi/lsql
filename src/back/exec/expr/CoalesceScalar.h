#pragma once

#include "back/exec/expr/Scalar.h"

namespace lsql::back::exec {

class CoalesceScalar : public Scalar {
 public:
    CoalesceScalar(ValueType value_type, std::vector<Arc<Scalar>> args)
        : value_type_(value_type)
        , args_(std::move(args)) {}

    FieldSet requiredFields() const override {
        FieldSet set;
        for (auto&& arg : args_) {
            set.merge(arg->requiredFields());
        }
        return set;
    }

    ValueType valueType() const override { return value_type_; }

    Value eval(const back::exec::Record& record) const override {
        for (auto&& arg : args_) {
            if (Value val = arg->eval(record); val != vnull) {
                return val;
            }
        }
        return vnull;
    }

 private:
    ValueType value_type_;
    std::vector<Arc<Scalar>> args_;
};

}  // namespace lsql::back::exec

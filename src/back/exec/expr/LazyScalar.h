#pragma once

#include "back/exec/expr/Scalar.h"
#include "back/exec/expr/ScalarContext.h"

namespace lsql::back::exec {

class LazyScalar : public Scalar {
 public:
    LazyScalar(ScalarContext::Id id, ScalarPtr scalar, Arc<ScalarContext> context)
        : id_(id)
        , scalar_(std::move(scalar))
        , context_(std::move(context)) {}

    FieldSet requiredFields() const override { return scalar_->requiredFields(); }

    ValueType valueType() const override { return scalar_->valueType(); }

    Value eval(const Record& record) const override {
        auto& value = context_->value(id_);
        if (!value.has_value()) {
            value = scalar_->eval(record);
        }
        return *value;
    }

 private:
    ScalarContext::Id id_;
    ScalarPtr scalar_;
    Arc<ScalarContext> context_;
};

}  // namespace lsql::back::exec

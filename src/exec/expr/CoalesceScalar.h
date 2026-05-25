#pragma once

#include "exec/expr/Scalar.h"

namespace lsql::exec {

class CoalesceScalar : public Scalar, public std::enable_shared_from_this<CoalesceScalar> {
 public:
    explicit CoalesceScalar(std::vector<ScalarPtr> values) : values_(std::move(values)) {}

    ValueType valueType() const override { return values_.front()->valueType(); }

    FieldSet requiredFields() const override {
        FieldSet result = FieldSet::emptySet();
        for (auto&& value : values_) {
            result.merge(value->requiredFields());
        }
        return result;
    }

    Value eval(const exec::Record& record) const override {
        for (auto&& expr : values_) {
            if (auto value = expr->eval(record); value != null) {
                return value;
            }
        }
        return null;
    }

 private:
    std::vector<ScalarPtr> values_;
};

}  // namespace lsql::exec

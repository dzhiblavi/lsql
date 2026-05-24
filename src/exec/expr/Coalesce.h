#pragma once

#include "exec/expr/Expression.h"

namespace lsql::exec {

class Coalesce : public Expression, public std::enable_shared_from_this<Coalesce> {
 public:
    explicit Coalesce(std::vector<ExpressionPtr> values) : values_(std::move(values)) {}

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
    std::vector<ExpressionPtr> values_;
};

}  // namespace lsql::exec

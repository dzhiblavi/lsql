#pragma once

#include "back/exec/expr/Scalar.h"
#include "core/function/Executor.h"

#include <ranges>

namespace lsql::back::exec {

class FnCallScalar : public Scalar {
 public:
    FnCallScalar(ValueType value_type, Arc<func::Executor> executor, std::vector<Arc<Scalar>> args)
        : value_type_(value_type)
        , executor_(std::move(executor))
        , args_(std::move(args)) {
        values_.assign(args_.size(), vnull);
    }

    FieldSet requiredFields() const override {
        FieldSet set;
        for (auto&& arg : args_) {
            set.merge(arg->requiredFields());
        }
        return set;
    }

    ValueType valueType() const override { return value_type_; }

    Value eval(const back::exec::Record& record) const override {
        for (auto&& [arg, value] : std::views::zip(args_, values_)) {
            value = arg->eval(record);
        }
        return executor_->execute(values_);
    }

 private:
    ValueType value_type_;
    Arc<func::Executor> executor_;
    std::vector<Arc<Scalar>> args_;
    mutable std::vector<Value> values_;
};

}  // namespace lsql::back::exec

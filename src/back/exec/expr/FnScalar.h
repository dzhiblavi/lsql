#pragma once

#include "back/exec/expr/Scalar.h"
#include "core/function/Executor.h"

#include <ranges>

namespace lsql::back::exec {

template <func::Executor E>
class FnScalar : public Scalar {
 public:
    FnScalar(ValueType value_type, E executor, std::vector<Arc<Scalar>> args)
        : value_type_(value_type)
        , args_(std::move(args))
        , executor_(std::move(executor)) {
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
        return executor_.execute(values_);
    }

 private:
    ValueType value_type_;
    std::vector<Arc<Scalar>> args_;
    mutable std::vector<Value> values_;
    [[no_unique_address]] E executor_;
};

}  // namespace lsql::back::exec

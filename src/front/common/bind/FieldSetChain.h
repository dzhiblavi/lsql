#pragma once

#include "front/common/bound/RelationFieldSet.h"

namespace lsql::front::common::bind {

struct FieldSetChain {
    FieldSetChain(bound::RelationFieldSetPtr top, FieldSetChain* parent)
        : current_(top)
        , parent_(parent) {}

    bound::RelationFieldSetPtr top() { return current_; }

    std::optional<ValueType> typeOfSourceField(std::string_view name, FieldBindingPtr binding) {
        if (current_ == nullptr) {
            return std::nullopt;
        }

        auto fields = current_->fields();
        for (auto id : fields.fieldIds()) {
            if (binding->name(id) == name) {
                current_->require(id);
                return binding->type(id);
            }
        }

        auto id = binding->getOrAdd(name, ValueType::String);
        if (current_->canProvide(id)) {
            current_->require(id);
            return ValueType::String;
        }

        if (parent_ == nullptr) {
            return std::nullopt;
        }

        return parent_->typeOfSourceField(name, binding);
    }

 private:
    bound::RelationFieldSetPtr current_;
    FieldSetChain* parent_;
};

}  // namespace lsql::front::common::bind

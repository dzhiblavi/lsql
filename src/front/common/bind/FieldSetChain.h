#pragma once

#include "front/common/bound/FieldSetNode.h"

namespace lsql::front::common::bind {

struct FieldSetChain {
    FieldSetChain(bound::FieldSetNodePtr top, FieldSetChain* parent)
        : current_(top)
        , parent_(parent) {}

    bound::FieldSetNodePtr top() { return current_; }

    std::optional<ValueType> typeOfSourceField(std::string_view name, FieldBindingPtr binding) {
        if (current_ == nullptr) {
            return std::nullopt;
        }

        for (auto id : current_->fieldSet().fieldIds()) {
            if (binding->name(id) == name) {
                return binding->type(id);
            }
        }

        if (current_->hasUnknown()) {
            // Some sources do not know which fields they contain.
            // They are modeled like they contain fields of any name with String type.
            current_->addUnknown(binding->getOrAdd(name, ValueType::String));
            return ValueType::String;
        }

        if (parent_ == nullptr) {
            return std::nullopt;
        }

        return parent_->typeOfSourceField(name, binding);
    }

 private:
    bound::FieldSetNodePtr current_;
    FieldSetChain* parent_;
};

}  // namespace lsql::front::common::bind

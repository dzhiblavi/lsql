#pragma once

#include "front/FieldSetNode.h"

#include "util/require.h"

namespace lsql::front {

struct FieldSetChain {
    FieldSetChain(FieldSetNodePtr top, FieldSetChain* parent) : current_(top), parent_(parent) {}

    FieldSetNodePtr top() { return current_; }

    ValueType typeOfSourceField(std::string_view name, FieldBindingPtr binding) {
        if (current_ == nullptr) {
            throwError("unknown field {}", name);
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

        require(parent_ != nullptr, "unknown field: {}", name);
        return parent_->typeOfSourceField(name, binding);
    }

 private:
    FieldSetNodePtr current_;
    FieldSetChain* parent_;
};

}  // namespace lsql::front

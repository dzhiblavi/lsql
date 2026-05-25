#pragma once

#include "iface/sql/bound/FieldSetNode.h"

#include "core/require.h"

namespace lsql::iface::sql::bind {

struct FieldSetChain {
    FieldSetChain(bound::FieldSetNode* top, FieldSetChain* parent)
        : current_(top)
        , parent_(parent) {}

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
    bound::FieldSetNode* current_;
    FieldSetChain* parent_;
};

}  // namespace lsql::iface::sql::bind

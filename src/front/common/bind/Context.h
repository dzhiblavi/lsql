#pragma once

#include "front/common/bind/FieldSetChain.h"

#include "core/schema/Fields.h"
#include "util/Pinned.h"

namespace lsql::front::common::bind {

class Context {
    struct ScopedFieldSet : util::Pinned {
        FieldSetChain** slot;
        FieldSetChain* old;
        ~ScopedFieldSet() { *slot = old; }
    };

 public:
    explicit Context(FieldBindingPtr binding) : binding_(binding) {}

    FieldBindingPtr binding() { return binding_; }

    FieldSetChain& currFieldSet() {
        verify(curr_field_set_slot_);
        return *curr_field_set_slot_;
    }

    ScopedFieldSet scopedFieldSet(FieldSetChain* curr) {
        return ScopedFieldSet{
            .slot = &curr_field_set_slot_,
            .old = std::exchange(curr_field_set_slot_, curr),
        };
    }

    [[nodiscard]] bool insert(const std::string& name, bound::FieldSetNodePtr p) {
        if (named_.contains(name)) {
            return false;
        }
        named_[name] = p;
        return true;
    }

    bound::FieldSetNodePtr find(const std::string& name) {
        auto it = named_.find(name);
        return it == named_.end() ? nullptr : it->second;
    }

 private:
    FieldBindingPtr binding_;
    FieldSetChain* curr_field_set_slot_ = nullptr;
    std::unordered_map<std::string, bound::FieldSetNodePtr> named_;
};

}  // namespace lsql::front::common::bind

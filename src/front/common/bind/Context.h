#pragma once

#include "front/common/bind/FieldSetChain.h"

#include "core/Fields.h"
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

    void insert(const std::string& name, bound::FieldSetNodePtr p) {
        require(!named_.contains(name), "duplicate named pipeline '{}'", name);
        named_[name] = p;
    }

    bound::FieldSetNodePtr find(const std::string& name) {
        auto it = named_.find(name);
        require(it != named_.end(), "unknown named pipeline '{}'", name);
        return it->second;
    }

 private:
    FieldBindingPtr binding_;
    FieldSetChain* curr_field_set_slot_ = nullptr;
    std::unordered_map<std::string, bound::FieldSetNodePtr> named_;
};

}  // namespace lsql::front::common::bind

#pragma once

#include "iface/sql/bind/FieldSetChain.h"

#include "iface/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bound/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bound/Statement.h"    // IWYU pragma: keep

#include "core/Fields.h"
#include "util/Pinned.h"

namespace lsql::iface::sql::bind {

class Context {
    struct ScopedFieldSet : util::Pinned {
        FieldSetChain** slot;
        FieldSetChain* old;
        ~ScopedFieldSet() { *slot = old; }
    };

 public:
    explicit Context(FieldBindingPtr binding) : binding_(std::move(binding)) {}

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

    void insertRelation(const std::string& name, bound::Relation* rel) {
        require(!named_relations_.contains(name), "duplicate named relation '{}", name);
        named_relations_[name] = rel;
    }

    bound::Relation* findRelation(const std::string& name) {
        auto it = named_relations_.find(name);
        require(it != named_relations_.end(), "unknown named relation '{}'", name);
        return it->second;
    }

 private:
    FieldBindingPtr binding_;
    std::unordered_map<std::string, bound::Relation*> named_relations_;
    FieldSetChain* curr_field_set_slot_ = nullptr;
};

}  // namespace lsql::iface::sql::bind

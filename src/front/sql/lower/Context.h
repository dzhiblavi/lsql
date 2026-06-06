#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "core/Fields.h"
#include "util/Pinned.h"
#include "util/require.h"

namespace lsql::front::sql::lower {

class Context {
    struct ScopedRelation : util::Pinned {
        std::optional<ir::Relation>* slot;
        std::optional<ir::Relation> old;
        ~ScopedRelation() { *slot = std::move(old); }
    };

    struct ScopedFieldSet : util::Pinned {
        const FieldSet** slot;
        const FieldSet* old;
        ~ScopedFieldSet() { *slot = old; }
    };

 public:
    explicit Context(FieldBindingPtr binding) : binding_(std::move(binding)) {}

    FieldBindingPtr binding() { return binding_; }

    bool hasRelation() const { return curr_relation_slot_.has_value(); }

    ScopedRelation scopedRelation(ir::Relation curr) {
        return ScopedRelation{
            .slot = &curr_relation_slot_,
            .old = std::exchange(curr_relation_slot_, std::move(curr)),
        };
    }

    ir::Relation& currRelation() {
        verify(curr_relation_slot_.has_value());
        return *curr_relation_slot_;
    }

    ir::Relation pullRelation() {
        auto r = std::move(currRelation());
        curr_relation_slot_ = std::nullopt;
        return r;
    }

    void setRelation(ir::Relation r) {
        verify(!curr_relation_slot_.has_value());
        curr_relation_slot_.emplace(std::move(r));
    }

    ScopedFieldSet scopedFieldSet(const FieldSet* curr) {
        return ScopedFieldSet{
            .slot = &curr_field_set_slot_,
            .old = std::exchange(curr_field_set_slot_, curr),
        };
    }

    const FieldSet& currFieldSet() {
        verify(curr_field_set_slot_);
        return *curr_field_set_slot_;
    }

    void insert(const std::string& name, FieldSet rel) {
        require(!named_relations_.contains(name), "duplicate named relation '{}", name);
        named_relations_[name] = rel;
    }

    FieldSet find(const std::string& name) {
        auto it = named_relations_.find(name);
        require(it != named_relations_.end(), "unknown named relation '{}'", name);
        return it->second;
    }

 private:
    FieldBindingPtr binding_;
    std::unordered_map<std::string, FieldSet> named_relations_;
    std::optional<ir::Relation> curr_relation_slot_;
    const FieldSet* curr_field_set_slot_ = nullptr;
};

}  // namespace lsql::front::sql::lower

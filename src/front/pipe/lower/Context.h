#pragma once

#include "ir/Aggregates.h"  // IWYU pragma: keep
#include "ir/Relations.h"   // IWYU pragma: keep
#include "ir/Scalars.h"     // IWYU pragma: keep

#include "core/Fields.h"
#include "util/Pinned.h"
#include "util/require.h"

namespace lsql::front::pipe::lower {

// shared across all statements/relations/whatever it is in the program
class ContextBase {
 public:
    explicit ContextBase(FieldBindingPtr binding) : binding_(std::move(binding)) {}

    FieldBindingPtr binding() { return binding_; }

    void insert(const std::string& name, const Schema& schema) {
        require(!named_relations_.contains(name), "duplicate named relation '{}'", name);
        named_relations_[name] = schema;
    }

    Schema find(const std::string& name) {
        auto it = named_relations_.find(name);
        require(it != named_relations_.end(), "unknown named relation '{}'", name);
        return it->second;
    }

 private:
    FieldBindingPtr binding_;
    std::unordered_map<std::string, Schema> named_relations_;
};

// local to a statement/pipeline/relation
class Context {
    struct ScopedFieldSet : util::Pinned {
        const FieldSet** slot;
        const FieldSet* old;
        ~ScopedFieldSet() { *slot = old; }
    };

 public:
    explicit Context(ContextBase* base) : base_(base) {}

    FieldBindingPtr binding() { return base_->binding(); }
    Schema find(const std::string& name) { return base_->find(name); }

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

    void insert(const std::string& name, const Schema& schema) { base_->insert(name, schema); }

    bool hasRelation() const { return curr_relation_.has_value(); }

    ir::Relation& currRelation() {
        verify(curr_relation_.has_value());
        return *curr_relation_;
    }

    ir::Relation pullRelation() {
        auto r = std::move(currRelation());
        curr_relation_ = std::nullopt;
        return r;
    }

    void setRelation(ir::Relation r) {
        verify(!curr_relation_.has_value());
        curr_relation_.emplace(std::move(r));
    }

    Context subContext() const { return Context(base_); }

 private:
    ContextBase* base_;
    const FieldSet* curr_field_set_slot_ = nullptr;
    std::optional<ir::Relation> curr_relation_;
};

}  // namespace lsql::front::pipe::lower

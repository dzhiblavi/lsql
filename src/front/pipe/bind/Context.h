#pragma once

#include "front/FieldSetChain.h"

#include "core/Fields.h"
#include "util/Pinned.h"

namespace lsql::front::pipe::bind {

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

    void insertPipeline(const std::string& name, FieldSetNodePtr p) {
        require(!named_pipelines_.contains(name), "duplicate named pipeline '{}'", name);
        named_pipelines_[name] = p;
    }

    FieldSetNodePtr findPipeline(const std::string& name) {
        auto it = named_pipelines_.find(name);
        require(it != named_pipelines_.end(), "unknown named pipeline '{}'", name);
        return it->second;
    }

 private:
    FieldBindingPtr binding_;
    FieldSetChain* curr_field_set_slot_ = nullptr;
    std::unordered_map<std::string, FieldSetNodePtr> named_pipelines_;
};

}  // namespace lsql::front::pipe::bind

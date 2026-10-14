#pragma once

#include "front/common/bound/RelationFieldSet.h"

#include "util/verify.h"

#include <vector>

namespace lsql::front::common::bound {

class FixedRelationFieldSet : public RelationFieldSet {
 public:
    explicit FixedRelationFieldSet(FieldSet fields) : fields_(std::move(fields)) {}

    FieldSet fields() const override { return fields_; }
    bool canProvide(FieldId id) const override { return fields_.contains(id); }
    void require(FieldId id) override { verify(canProvide(id)); }

 private:
    FieldSet fields_;
};

class OpenRelationFieldSet : public RelationFieldSet {
 public:
    explicit OpenRelationFieldSet(ConstFieldBindingPtr binding) : binding_(std::move(binding)) {}

    FieldSet fields() const override { return fields_; }
    bool canProvide(FieldId id) const override { return binding_->type(id) == ValueType::String; }

    void require(FieldId id) override {
        verify(canProvide(id));
        fields_.add(id);
    }

 private:
    ConstFieldBindingPtr binding_;
    FieldSet fields_;
};

class CombinedRelationFieldSet : public RelationFieldSet {
 public:
    CombinedRelationFieldSet(FieldSet own_fields, std::vector<RelationFieldSetPtr> sources)
        : own_fields_(std::move(own_fields))
        , sources_(std::move(sources)) {
        for (const auto& source : sources_) {
            verify(source != nullptr);
        }
    }

    FieldSet fields() const override {
        auto fields = own_fields_;

        for (const auto& source : sources_) {
            fields.merge(source->fields());
        }

        return fields;
    }

    bool canProvide(FieldId id) const override {
        if (own_fields_.contains(id)) {
            return true;
        }

        for (const auto& source : sources_) {
            if (source->canProvide(id)) {
                return true;
            }
        }

        return false;
    }

    void require(FieldId id) override {
        verify(canProvide(id));

        if (own_fields_.contains(id)) {
            return;
        }

        for (const auto& source : sources_) {
            if (source->canProvide(id)) {
                source->require(id);
            }
        }
    }

 private:
    FieldSet own_fields_;
    std::vector<RelationFieldSetPtr> sources_;
};

}

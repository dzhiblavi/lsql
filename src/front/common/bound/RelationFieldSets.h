#pragma once

#include "front/common/bound/RelationFieldSet.h"

#include "util/verify.h"

#include <optional>
#include <string>
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

class AliasRelationFieldSet : public RelationFieldSet {
 public:
    AliasRelationFieldSet(std::string alias, RelationFieldSetPtr source, FieldBindingPtr binding)
        : prefix_(std::move(alias) + ".")
        , source_(std::move(source))
        , binding_(std::move(binding)) {}

    FieldSet fields() const override {
        auto source_fields = source_->fields();
        FieldSet fields;
        for (auto id : source_fields.fieldIds()) {
            auto name = prefix_ + std::string(binding_->name(id));
            fields.add(binding_->getOrAdd(name, binding_->type(id)));
        }
        return fields;
    }

    bool canProvide(FieldId id) const override {
        auto source_id = sourceField(id);
        return source_id && source_->canProvide(*source_id);
    }

    void require(FieldId id) override {
        auto source_id = sourceField(id);
        verify(source_id.has_value());
        source_->require(*source_id);
    }

 private:
    std::optional<FieldId> sourceField(FieldId id) const {
        auto name = binding_->name(id);
        if (!name.starts_with(prefix_) || name.size() == prefix_.size()) {
            return std::nullopt;
        }
        auto source_name = std::string(name.substr(prefix_.size()));
        return binding_->getOrAdd(source_name, binding_->type(id));
    }

    std::string prefix_;
    RelationFieldSetPtr source_;
    FieldBindingPtr binding_;
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

}  // namespace lsql::front::common::bound

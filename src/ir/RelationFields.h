#pragma once

#include "core/Fields.h"
#include "core/ValueType.h"
#include "core/verify.h"

#include <unordered_map>

namespace lsql::ir {

class RelationFields {
 public:
    void add(FieldId id, ValueType type) {
        auto it = fields_.find(id);
        if (it == fields_.end()) {
            fields_.emplace(id, type);
            return;
        }

        verify(it->second == type);
    }

    bool contains(FieldId id, ValueType type) const {
        if (containsField(id, type)) {
            return true;
        }

        if (unknown_) {
            return type == ValueType::String;
        }

        return false;
    }

    bool hasUnknown() const { return unknown_; }

    const std::unordered_map<FieldId, ValueType>& fields() const { return fields_; };

    void merge(const RelationFields& other) {
        for (auto&& [name, type] : other.fields_) {
            if (auto it = fields_.find(name); it != fields_.end()) {
                if (it->second != type) {
                    throw std::runtime_error(std::format("different fields types for {}", name));
                }
            }
            fields_[name] = type;
        }

        unknown_ |= other.unknown_;
    }

    void setUnknown() { unknown_ = true; }

    static RelationFields unknownSet() { return {true, {}}; }
    static RelationFields emptySet() { return {false, {}}; }

    static RelationFields merge(RelationFields a, const RelationFields& b) {
        a.merge(b);
        return a;
    }

 private:
    RelationFields(bool unknown, std::unordered_map<FieldId, ValueType> fields)
        : unknown_(unknown)
        , fields_(std::move(fields)) {}

    bool containsField(FieldId id, ValueType type) const {
        auto it = fields_.find(id);
        if (it == fields_.end()) {
            return false;
        }
        return it->second == type;
    }

    // has any field with type String
    bool unknown_ = false;

    // concrete fields that this relation provides
    std::unordered_map<FieldId, ValueType> fields_;
};

}  // namespace lsql::iface::sql::bind

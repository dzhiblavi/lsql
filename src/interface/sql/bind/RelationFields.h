#pragma once

#include "core/ValueType.h"
#include "core/verify.h"

#include <unordered_map>

namespace lsql::iface::sql::bind {

class RelationFields {
 public:
    void add(std::string name, ValueType type) {
        auto it = fields_.find(name);
        if (it == fields_.end()) {
            fields_.emplace(std::move(name), type);
            return;
        }

        verify(it->second == type);
    }

    bool contains(const std::string& name, ValueType type) const {
        if (containsField(name, type)) {
            return true;
        }

        if (unknown_) {
            return type == ValueType::String;
        }

        return false;
    }

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
    RelationFields(bool unknown, std::unordered_map<std::string, ValueType> fields)
        : unknown_(unknown)
        , fields_(std::move(fields)) {}

    bool containsField(const std::string& name, ValueType type) const {
        auto it = fields_.find(name);
        if (it == fields_.end()) {
            return false;
        }
        return it->second == type;
    }

    // has any field with type String
    bool unknown_ = false;

    // concrete fields that this relation provides
    std::unordered_map<std::string, ValueType> fields_;
};

}  // namespace lsql::iface::sql::bind

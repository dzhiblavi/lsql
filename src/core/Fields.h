#pragma once

#include "core/ValueType.h"
#include "util/verify.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace lsql {

using FieldId = uint32_t;

static constexpr FieldId UnknownFieldId = 0;

class FieldBinding {
 public:
    FieldBinding() = default;

    FieldId addAnonymous(std::string_view prefix, ValueType type) {
        return add(std::format("${}_{}", prefix, next_id_), type);
    }

    FieldId add(std::string_view name, ValueType type) {
        verify(!hasField(name, type));

        auto id = next_id_++;
        names_.emplace(id, name);
        types_.emplace(id, type);
        ids_[magic_enum::enum_underlying(type)].emplace(name, id);
        return id;
    }

    FieldId getOrAdd(std::string_view name, ValueType type) {
        return hasField(name, type) ? id(name, type) : add(name, type);
    }

    std::string_view name(FieldId id) const {
        auto it = names_.find(id);
        verify(it != names_.end());
        return it->second;
    }

    ValueType type(FieldId id) const {
        auto it = types_.find(id);
        verify(it != types_.end());
        return it->second;
    }

    bool hasField(std::string_view name, ValueType type) const {
        return ids_[magic_enum::enum_underlying(type)].contains(name);
    }

    FieldId id(std::string_view name, ValueType type) const {
        auto&& ids = ids_[magic_enum::enum_underlying(type)];
        auto it = ids.find(name);
        if (it != ids.end()) {
            return it->second;
        }
        return UnknownFieldId;
    }

 private:
    FieldId next_id_ = UnknownFieldId + 1;
    absl::flat_hash_map<FieldId, std::string> names_;
    absl::flat_hash_map<FieldId, ValueType> types_;
    std::array<absl::flat_hash_map<std::string, FieldId>, magic_enum::enum_count<ValueType>()> ids_;
};

using FieldBindingPtr = std::shared_ptr<FieldBinding>;
using ConstFieldBindingPtr = std::shared_ptr<const FieldBinding>;

class FieldSet {
 public:
    FieldSet() = default;

    bool contains(FieldId id) const { return fields_.contains(id); }
    bool empty() const { return fields_.empty(); }
    size_t size() const { return fields_.size(); }

    const std::unordered_set<FieldId>& fieldIds() const { return fields_; };

    void add(FieldId id) { fields_.insert(id); }

    void merge(const FieldSet& other) {
        fields_.insert(other.fields_.begin(), other.fields_.end());
    }

    static FieldSet withField(FieldId id) { return FieldSet({id}); }
    static FieldSet emptySet() { return FieldSet{{}}; }

    static FieldSet merge(FieldSet a, const FieldSet& b) {
        a.merge(b);
        return a;
    }

 private:
    explicit FieldSet(std::unordered_set<FieldId> fields) : fields_(std::move(fields)) {}

    std::unordered_set<FieldId> fields_;
};

inline std::string to_string(FieldId id, const FieldBinding& binding) {
    return std::format("{}({},{})", binding.name(id), id, magic_enum::enum_name(binding.type(id)));
}

inline std::string to_string(const FieldSet& fields, const FieldBinding& binding) {
    if (fields.fieldIds().empty()) {
        return "[]";
    }

    std::stringstream ss;
    ss << '[';
    for (auto&& id : fields.fieldIds()) {
        ss << to_string(id, binding) << ',';
    }
    ss.seekp(-1, std::ios_base::end);  // remove last ','
    ss << ']';
    return ss.str();
}

}  // namespace lsql

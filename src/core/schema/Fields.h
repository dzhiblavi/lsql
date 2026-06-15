#pragma once

#include "core/value/ValueType.h"
#include "util/verify.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace lsql {

using FieldId = uint32_t;
using SlotId = uint32_t;

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

    bool operator==(const FieldSet&) const = default;

    static FieldSet withField(FieldId id) { return FieldSet({id}); }
    static FieldSet emptySet() { return FieldSet(); }

    static FieldSet merge(FieldSet a, const FieldSet& b) {
        a.merge(b);
        return a;
    }

    static FieldSet intersection(FieldSet a, const FieldSet& b) {
        for (auto it = a.fields_.begin(); it != a.fields_.end();) {
            if (b.fields_.contains(*it)) {
                ++it;
            } else {
                it = a.fields_.erase(it);
            }
        }
        return a;
    }

 private:
    explicit FieldSet(std::unordered_set<FieldId> fields) : fields_(std::move(fields)) {}

    std::unordered_set<FieldId> fields_;
};

class Schema {
 public:
    Schema() = default;

    bool contains(FieldId id) const { return slots_.contains(id); }

    std::optional<SlotId> slot(FieldId id) const {
        auto it = slots_.find(id);
        return it == slots_.end() ? std::nullopt : std::optional(it->second);
    }

    SlotId append(FieldId id) {
        verify(!contains(id), "duplicate field id {}", id);
        auto slot = next_slot_++;
        slots_.emplace(id, slot);
        return slot;
    }

    std::vector<FieldId> fieldIds() const {
        std::vector<FieldId> result;
        verify(slots_.size() == next_slot_);
        result.resize(next_slot_, UnknownFieldId);
        for (auto&& [id, index] : slots_) {
            verify(0 <= index && index < result.size());
            verify(result[index] == UnknownFieldId);
            result[index] = id;
        }
        return result;
    }

    bool operator==(const Schema&) const = default;

    FieldSet fieldSet() const {
        FieldSet s;
        for (FieldId id : fieldIds()) {
            s.add(id);
        }
        return s;
    }

    size_t columns() const { return next_slot_; }

    static Schema fromFieldSet(const FieldSet& set) {
        Schema s;
        for (auto&& id : set.fieldIds()) {
            s.append(id);
        }
        return s;
    }

    static Schema concat(Schema a, const Schema& b) {
        for (FieldId id : b.fieldIds()) {
            a.append(id);
        }
        return a;
    }

    static Schema withField(FieldId id) { return fromFieldSet(FieldSet::withField(id)); }

 private:
    std::unordered_map<FieldId, SlotId> slots_;
    SlotId next_slot_ = 0;
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

inline std::string to_string(const Schema& schema, const FieldBinding& binding) {
    if (schema.columns() == 0) {
        return "[]";
    }

    std::stringstream ss;
    ss << '[';
    for (auto&& id : schema.fieldIds()) {
        ss << to_string(id, binding) << ',';
    }
    ss.seekp(-1, std::ios_base::end);  // remove last ','
    ss << ']';
    return ss.str();
}

}  // namespace lsql

#pragma once

#include "core/schema/FieldSet.h"
#include "core/schema/types.h"

#include "util/verify.h"

#include <cstdint>
#include <string>

namespace lsql {

class Schema {
 public:
    Schema() = default;

    bool contains(FieldId id) const { return slots_.contains(id); }

    std::optional<SlotId> slot(FieldId id) const {
        auto it = slots_.find(id);
        return it == slots_.end() ? std::nullopt : std::optional(it->second);
    }

    SlotId append(FieldId id) {
        verify(!contains(id), "duplicate field id {}", uint32_t(id));
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
    uint32_t next_slot_ = 0;
};

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

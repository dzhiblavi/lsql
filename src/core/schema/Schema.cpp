#include "core/schema/Schema.h"

#include "util/verify.h"

namespace lsql {

bool Schema::contains(FieldId id) const {
    return slots_.contains(id);
}

std::optional<SlotId> Schema::slot(FieldId id) const {
    auto it = slots_.find(id);
    return it == slots_.end() ? std::nullopt : std::optional(it->second);
}

SlotId Schema::append(FieldId id) {
    verify(!contains(id), "duplicate field id {}", uint32_t(id));
    auto slot = next_slot_++;
    slots_.emplace(id, slot);
    return slot;
}

std::vector<FieldId> Schema::fieldIds() const {
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

FieldSet Schema::fieldSet() const {
    FieldSet s;
    for (FieldId id : fieldIds()) {
        s.add(id);
    }
    return s;
}

size_t Schema::columns() const {
    return next_slot_;
}

Schema Schema::fromFieldSet(const FieldSet& set) {
    Schema s;
    for (auto&& id : set.fieldIds()) {
        s.append(id);
    }
    return s;
}

Schema Schema::concat(Schema a, const Schema& b) {
    for (FieldId id : b.fieldIds()) {
        a.append(id);
    }
    return a;
}

Schema Schema::withField(FieldId id) {
    return fromFieldSet(FieldSet::withField(id));
}

std::string to_string(const Schema& schema, const FieldBinding& binding) {
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

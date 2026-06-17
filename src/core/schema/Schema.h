#pragma once

#include "core/schema/FieldSet.h"
#include "core/schema/types.h"

#include <cstdint>
#include <string>

namespace lsql {

class Schema {
 public:
    Schema() = default;

    SlotId append(FieldId id);
    bool contains(FieldId id) const;
    bool contains(const FieldSet& fields) const;
    std::optional<SlotId> slot(FieldId id) const;
    std::vector<FieldId> fieldIds() const;
    FieldSet fieldSet() const;
    size_t columns() const;

    static Schema fromFieldSet(const FieldSet& set);
    static Schema concat(Schema a, const Schema& b);
    static Schema withField(FieldId id);

    bool operator==(const Schema&) const = default;

 private:
    std::unordered_map<FieldId, SlotId> slots_;
    uint32_t next_slot_ = 0;
};

std::string to_string(const Schema& schema, const FieldBinding& binding);

}  // namespace lsql

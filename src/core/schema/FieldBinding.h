#pragma once

#include "core/schema/types.h"
#include "core/value/ValueType.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace lsql {

class FieldBinding {
 public:
    FieldBinding() = default;

    FieldId addAnonymous(std::string_view prefix, ValueType type);
    FieldId add(std::string_view name, ValueType type);
    FieldId getOrAdd(std::string_view name, ValueType type);
    std::string_view name(FieldId id) const;
    ValueType type(FieldId id) const;

    bool hasField(std::string_view name, ValueType type) const;
    FieldId id(std::string_view name, ValueType type) const;

 private:
    uint32_t next_id_ = UnknownFieldId + 1;
    absl::flat_hash_map<FieldId, std::string> names_;
    absl::flat_hash_map<FieldId, ValueType> types_;
    std::array<absl::flat_hash_map<std::string, FieldId>, magic_enum::enum_count<ValueType>()> ids_;
};

using FieldBindingPtr = std::shared_ptr<FieldBinding>;
using ConstFieldBindingPtr = std::shared_ptr<const FieldBinding>;

std::string to_string(FieldId id, const FieldBinding& binding);

}  // namespace lsql

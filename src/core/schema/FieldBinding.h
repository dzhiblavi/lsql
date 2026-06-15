#pragma once

#include "core/schema/types.h"
#include "core/value/ValueType.h"

#include "util/verify.h"

#include <absl/container/flat_hash_map.h>
#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace lsql {

class FieldBinding {
 public:
    FieldBinding() = default;

    FieldId addAnonymous(std::string_view prefix, ValueType type) {
        return add(std::format("${}_{}", prefix, uint32_t(next_id_)), type);
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
    uint32_t next_id_ = UnknownFieldId + 1;
    absl::flat_hash_map<FieldId, std::string> names_;
    absl::flat_hash_map<FieldId, ValueType> types_;
    std::array<absl::flat_hash_map<std::string, FieldId>, magic_enum::enum_count<ValueType>()> ids_;
};

using FieldBindingPtr = std::shared_ptr<FieldBinding>;
using ConstFieldBindingPtr = std::shared_ptr<const FieldBinding>;

inline std::string to_string(FieldId id, const FieldBinding& binding) {
    return std::format(
        "{}({},{})", binding.name(id), uint32_t(id), magic_enum::enum_name(binding.type(id)));
}

}  // namespace lsql

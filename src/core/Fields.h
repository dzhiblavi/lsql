#pragma once

#include "core/verify.h"
#include <absl/container/flat_hash_map.h>

#include <cstdint>
#include <memory>
#include <string>

namespace lsql {

using FieldId = uint32_t;

static constexpr FieldId UnknownFieldId = 0;

class FieldBinding {
 public:
    FieldBinding() = default;

    FieldId addAnonymous() { return add(std::format("$anon_{}", next_id_)); }

    FieldId add(std::string_view name) {
        std::string sname(name);
        verify(!hasField(sname));
        FieldId id = next_id_++;
        names_.emplace(id, sname);
        ids_.emplace(std::move(sname), id);
        return id;
    }

    FieldId getOrAdd(std::string_view name) { return hasField(name) ? id(name) : add(name); }

    std::string_view name(FieldId id) const {
        auto it = names_.find(id);
        verify(it != names_.end());
        return it->second;
    }

    bool hasField(std::string_view name) const { return ids_.contains(name); }

    FieldId id(std::string_view name) const {
        auto it = ids_.find(name);
        if (it != ids_.end()) {
            return it->second;
        }
        return UnknownFieldId;
    }

 private:
    FieldId next_id_ = 1;
    absl::flat_hash_map<FieldId, std::string> names_;
    absl::flat_hash_map<std::string, FieldId> ids_;
};

using FieldBindingPtr = std::shared_ptr<FieldBinding>;
using ConstFieldBindingPtr = std::shared_ptr<const FieldBinding>;

}  // namespace lsql

#include "core/schema/FieldBinding.h"

#include "util/verify.h"

namespace lsql {

FieldId FieldBinding::addAnonymous(std::string_view prefix, ValueType type) {
    return add(std::format("${}_{}", prefix, uint32_t(next_id_)), type);
}

FieldId FieldBinding::add(std::string_view name, ValueType type) {
    verify(!hasField(name, type));

    auto id = next_id_++;
    names_.emplace(id, name);
    types_.emplace(id, type);
    ids_[magic_enum::enum_underlying(type)].emplace(name, id);
    return id;
}

FieldId FieldBinding::getOrAdd(std::string_view name, ValueType type) {
    return hasField(name, type) ? id(name, type) : add(name, type);
}

std::string_view FieldBinding::name(FieldId id) const {
    auto it = names_.find(id);
    verify(it != names_.end());
    return it->second;
}

ValueType FieldBinding::type(FieldId id) const {
    auto it = types_.find(id);
    verify(it != types_.end());
    return it->second;
}

bool FieldBinding::hasField(std::string_view name, ValueType type) const {
    return ids_[magic_enum::enum_underlying(type)].contains(name);
}

FieldId FieldBinding::id(std::string_view name, ValueType type) const {
    auto&& ids = ids_[magic_enum::enum_underlying(type)];
    auto it = ids.find(name);
    if (it != ids.end()) {
        return it->second;
    }
    return UnknownFieldId;
}

std::string to_string(FieldId id, const FieldBinding& binding) {
    return std::format(
        "{}({},{})", binding.name(id), uint32_t(id), magic_enum::enum_name(binding.type(id)));
}

}  // namespace lsql

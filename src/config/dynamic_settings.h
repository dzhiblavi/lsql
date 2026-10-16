#pragma once

#include "util/concepts.h"
#include "util/verify.h"

#include <magic_enum/magic_enum.hpp>

#include <array>

namespace lsql::config {

template <util::Enum E, typename Self>
struct DynamicSettings {
    DynamicSettings() { self().setDefaults(); }

    void set(E setting, int64_t value) {
        self().validate(setting, value);

        auto index = magic_enum::enum_index(setting);
        verify(index.has_value());

        values_[*index] = value;
    }

    int64_t get(E setting) const {
        auto index = magic_enum::enum_index(setting);
        verify(index.has_value());

        return values_[*index];
    }

 private:
    Self& self() { return *static_cast<Self*>(this); }

    std::array<int64_t, magic_enum::enum_count<E>()> values_;
};

}  // namespace lsql::config

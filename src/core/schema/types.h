#pragma once

#include <cstdint>
#include <functional>

namespace lsql {

struct FieldId {
    constexpr FieldId(uint32_t id) : int_(id) {}
    constexpr operator uint32_t() const { return int_; }

 private:
    uint32_t int_;
};

inline constexpr FieldId UnknownFieldId = FieldId(0);

struct SlotId {
    constexpr SlotId(uint32_t id) : int_(id) {}
    constexpr operator uint32_t() const { return int_; }

 private:
    uint32_t int_;
};

}  // namespace lsql

namespace std {

template <>
struct hash<lsql::FieldId> {
    size_t operator()(lsql::FieldId id) const noexcept { return std::hash<uint32_t>{}(id); }
};

template <>
struct hash<lsql::SlotId> {
    size_t operator()(lsql::SlotId slot) const noexcept { return std::hash<uint32_t>{}(slot); }
};

}  // namespace std

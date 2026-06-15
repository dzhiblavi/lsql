#pragma once

#include "core/schema/FieldBinding.h"
#include "core/schema/types.h"

#include <string>

namespace lsql {

class FieldSet {
 public:
    FieldSet() = default;

    bool contains(FieldId id) const;
    bool empty() const;
    size_t size() const;
    const std::unordered_set<FieldId>& fieldIds() const;
    void add(FieldId id);
    void merge(const FieldSet& other);

    static FieldSet withField(FieldId id);
    static FieldSet emptySet();
    static FieldSet merge(FieldSet a, const FieldSet& b);
    static FieldSet intersection(FieldSet a, const FieldSet& b);

    bool operator==(const FieldSet&) const = default;

 private:
    explicit FieldSet(std::unordered_set<FieldId> fields);

    std::unordered_set<FieldId> fields_;
};

std::string to_string(const FieldSet& fields, const FieldBinding& binding);

}  // namespace lsql

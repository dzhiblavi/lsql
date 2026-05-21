#pragma once

#include "core/Fields.h"

#include <unordered_set>

namespace lsql::ir {

class RelationFields {
 public:
    bool containsField(FieldId id) const { return fields_.contains(id); }
    const std::unordered_set<FieldId>& fieldIds() const { return fields_; };

    void add(FieldId id) { fields_.insert(id); }

    void merge(const RelationFields& other) {
        fields_.insert(other.fields_.begin(), other.fields_.end());
    }

    static RelationFields emptySet() { return RelationFields{{}}; }

    static RelationFields merge(RelationFields a, const RelationFields& b) {
        a.merge(b);
        return a;
    }

    static RelationFields withField(FieldId id) {
        auto r = RelationFields({id});
        r.add(id);
        return r;
    }

 private:
    explicit RelationFields(std::unordered_set<FieldId> fields) : fields_(std::move(fields)) {}

    std::unordered_set<FieldId> fields_;
};

}  // namespace lsql::ir

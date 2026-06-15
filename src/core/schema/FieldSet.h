#pragma once

#include "core/schema/FieldBinding.h"
#include "core/schema/types.h"

#include <string>

namespace lsql {

class FieldSet {
 public:
    FieldSet() = default;

    bool contains(FieldId id) const { return fields_.contains(id); }
    bool empty() const { return fields_.empty(); }
    size_t size() const { return fields_.size(); }

    const std::unordered_set<FieldId>& fieldIds() const { return fields_; };

    void add(FieldId id) { fields_.insert(id); }

    void merge(const FieldSet& other) {
        fields_.insert(other.fields_.begin(), other.fields_.end());
    }

    bool operator==(const FieldSet&) const = default;

    static FieldSet withField(FieldId id) { return FieldSet({id}); }
    static FieldSet emptySet() { return FieldSet(); }

    static FieldSet merge(FieldSet a, const FieldSet& b) {
        a.merge(b);
        return a;
    }

    static FieldSet intersection(FieldSet a, const FieldSet& b) {
        for (auto it = a.fields_.begin(); it != a.fields_.end();) {
            if (b.fields_.contains(*it)) {
                ++it;
            } else {
                it = a.fields_.erase(it);
            }
        }
        return a;
    }

 private:
    explicit FieldSet(std::unordered_set<FieldId> fields) : fields_(std::move(fields)) {}

    std::unordered_set<FieldId> fields_;
};

inline std::string to_string(const FieldSet& fields, const FieldBinding& binding) {
    if (fields.fieldIds().empty()) {
        return "[]";
    }

    std::stringstream ss;
    ss << '[';
    for (auto&& id : fields.fieldIds()) {
        ss << to_string(id, binding) << ',';
    }
    ss.seekp(-1, std::ios_base::end);  // remove last ','
    ss << ']';
    return ss.str();
}

}  // namespace lsql

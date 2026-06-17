#include "core/schema/FieldSet.h"

namespace lsql {

bool FieldSet::contains(FieldId id) const {
    return fields_.contains(id);
}

bool FieldSet::empty() const {
    return fields_.empty();
}

size_t FieldSet::size() const {
    return fields_.size();
}

const std::unordered_set<FieldId>& FieldSet::fieldIds() const {
    return fields_;
}

void FieldSet::add(FieldId id) {
    fields_.insert(id);
}

void FieldSet::remove(FieldId id) {
    fields_.erase(id);
}

void FieldSet::merge(const FieldSet& other) {
    fields_.insert(other.fields_.begin(), other.fields_.end());
}

FieldSet FieldSet::withField(FieldId id) {
    return FieldSet({id});
}

FieldSet FieldSet::emptySet() {
    return FieldSet();
}

FieldSet FieldSet::merge(FieldSet a, const FieldSet& b) {
    a.merge(b);
    return a;
}

FieldSet FieldSet::intersection(FieldSet a, const FieldSet& b) {
    for (auto it = a.fields_.begin(); it != a.fields_.end();) {
        if (b.fields_.contains(*it)) {
            ++it;
        } else {
            it = a.fields_.erase(it);
        }
    }
    return a;
}

FieldSet::FieldSet(std::unordered_set<FieldId> fields) : fields_(std::move(fields)) {
}

std::string to_string(const FieldSet& fields, const FieldBinding& binding) {
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

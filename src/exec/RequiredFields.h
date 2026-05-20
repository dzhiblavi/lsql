#pragma once

#include "core/Fields.h"
#include "core/verify.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::exec {

class RequiredFields {
 public:
    bool all() const { return all_; }
    bool empty() const { return !all_ && ids_.empty(); }
    bool requiresField(FieldId id) const { return all_ || ids_.contains(id); }

    const absl::flat_hash_set<FieldId>& ids() const {
        verify(!all());
        return ids_;
    }

    void require(FieldId id) { ids_.insert(id); }

    void merge(const RequiredFields& rhs) {
        if (all() || rhs.all()) {
            all_ = true;
            ids_.clear();
            return;
        }

        ids_.insert(rhs.ids_.begin(), rhs.ids_.end());
    }

    static RequiredFields withNone() { return RequiredFields(false, {}); }

    static RequiredFields withAll() { return RequiredFields(true, {}); }

    static RequiredFields withFields(absl::flat_hash_set<FieldId> ids) {
        return RequiredFields(false, std::move(ids));
    }

    static RequiredFields merge(RequiredFields a, const RequiredFields& b) {
        a.merge(b);
        return a;
    }

 private:
    RequiredFields(bool all, absl::flat_hash_set<FieldId> ids) : all_(all), ids_(std::move(ids)) {}

    bool all_ = false;
    absl::flat_hash_set<FieldId> ids_;
};

inline std::string to_string(const RequiredFields& fields, const FieldBinding& binding) {
    if (fields.all()) {
        return "all";
    }
    if (fields.empty()) {
        return "none";
    }

    std::stringstream ss;
    for (auto&& id : fields.ids()) {
        ss << std::format("{}({}),", binding.name(id), id);
    }
    ss.seekp(-1, std::ios_base::end);  // remove last ','
    return ss.str();
}

}  // namespace lsql::exec

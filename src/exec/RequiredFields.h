#pragma once

#include "core/verify.h"
#include <absl/container/flat_hash_set.h>

namespace lsql::exec {

class RequiredFields {
 public:
    bool all() const { return all_; }
    bool empty() const { return !all_ && names_.empty(); }
    bool requiresField(std::string_view name) const { return all_ || names_.contains(name); }

    const absl::flat_hash_set<std::string>& names() const {
        verify(!all());
        return names_;
    }

    void require(std::string name) { names_.insert(std::move(name)); }

    void merge(const RequiredFields& rhs) {
        if (all() || rhs.all()) {
            all_ = true;
            names_.clear();
            return;
        }

        names_.insert(rhs.names_.begin(), rhs.names_.end());
    }

    static RequiredFields withNone() { return RequiredFields(false, {}); }

    static RequiredFields withAll() { return RequiredFields(true, {}); }

    static RequiredFields withFields(absl::flat_hash_set<std::string> names) {
        return RequiredFields(false, std::move(names));
    }

    static RequiredFields merge(RequiredFields a, const RequiredFields& b) {
        a.merge(b);
        return a;
    }

 private:
    RequiredFields(bool all, absl::flat_hash_set<std::string> names)
        : all_(all)
        , names_(std::move(names)) {}

    bool all_ = false;
    absl::flat_hash_set<std::string> names_;
};

inline std::string to_string(const RequiredFields& fields) {
    if (fields.all()) {
        return "all";
    }
    if (fields.empty()) {
        return "none";
    }

    std::stringstream ss;
    for (auto&& name : fields.names()) {
        ss << name << ',';
    }
    ss.seekp(-1, std::ios_base::end);  // remove last ','
    return ss.str();
}

}  // namespace lsql::exec

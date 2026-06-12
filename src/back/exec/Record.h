#pragma once

#include "profiling/global.h"

#include "core/Fields.h"
#include "core/Value.h"
#include "core/types.h"

#include <absl/container/flat_hash_set.h>

namespace lsql::back::exec {

class Record : public std::enable_shared_from_this<Record> {
 public:
    using ids_t = absl::flat_hash_set<FieldId>;

    virtual ~Record() = default;

    virtual const Value& value(FieldId id) const = 0;

    Arc<const Record> clone() const {
        if (!weak_from_this().expired()) {
            prof::addCounter("record.share");
            return shared_from_this();
        }

        prof::addCounter("record.clone");
        return cloneImpl();
    }

 private:
    virtual Arc<const Record> cloneImpl() const = 0;
};

using RecordPtr = Arc<Record>;
using ConstRecordPtr = Arc<const Record>;

class EmptyRecord : public Record {
 public:
    EmptyRecord() = default;

    const Value& value(FieldId) const override { return vnull; }

    static ConstRecordPtr instance() {
        static ConstRecordPtr record = arc<EmptyRecord>();
        return record;
    }

 private:
    Arc<const Record> cloneImpl() const override { return instance(); }
};

using RecordRef = std::variant<const Record*, ConstRecordPtr>;

inline ConstRecordPtr pin(const RecordRef& ref) {
    return std::visit(
        util::Overloaded{
            [](const Record* record) { return record->clone(); },
            [](ConstRecordPtr record) { return record; },
        },
        ref);
}

inline const Record* get(const RecordRef& ref) {
    return std::visit(
        util::Overloaded{
            [](const Record* record) { return record; },
            [](ConstRecordPtr record) { return record.get(); },
        },
        ref);
}

}  // namespace lsql::back::exec

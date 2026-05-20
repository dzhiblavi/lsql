#pragma once

#include "core/Fields.h"
#include "core/Value.h"

#include <absl/container/flat_hash_set.h>

#include <memory>

namespace lsql::exec {

class Record : public std::enable_shared_from_this<Record> {
 public:
    using ids_t = absl::flat_hash_set<FieldId>;

    virtual ~Record() = default;

    virtual ids_t ids() const = 0;

    virtual Value value(FieldId id) const = 0;

    std::shared_ptr<const Record> clone() const {
        if (!weak_from_this().expired()) {
            return shared_from_this();
        }

        return cloneImpl();
    }

 private:
    virtual std::shared_ptr<const Record> cloneImpl() const = 0;
};

using RecordPtr = std::shared_ptr<Record>;
using ConstRecordPtr = std::shared_ptr<const Record>;

class EmptyRecord : public Record {
 public:
    EmptyRecord() = default;

    ids_t ids() const override { return {}; }

    Value value(FieldId) const override { return null; }

    static ConstRecordPtr instance() {
        static ConstRecordPtr record = std::make_shared<EmptyRecord>();
        return record;
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override { return instance(); }
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

}  // namespace lsql::exec

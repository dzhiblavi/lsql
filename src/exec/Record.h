#pragma once

#include "core/Value.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lsql::exec {

class Record : public std::enable_shared_from_this<Record> {
 public:
    using values_t = std::unordered_map<std::string, Value>;

    virtual ~Record() = default;

    virtual values_t values() const = 0;
    virtual Value value(std::string_view name) const = 0;

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

    values_t values() const override { return {}; }
    Value value(std::string_view /*name*/) const override { return null; }

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

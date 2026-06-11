#pragma once

#include "back/exec/Record.h"

namespace lsql::back::exec {

struct MockRecord : Record {
    using values_t = std::unordered_map<FieldId, Value>;

    explicit MockRecord(values_t values) : values_(std::move(values)) {}

    Value value(FieldId id) const override {
        auto it = values_.find(id);
        return it == values_.end() ? null : it->second;
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<MockRecord>(*this);
    }

    values_t values_;
};

}  // namespace lsql::back::exec

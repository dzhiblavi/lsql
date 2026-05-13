#pragma once

#include "exec/Record.h"

namespace lsql::exec {

struct MockRecord : Record {
    explicit MockRecord(values_t values) : values_(std::move(values)) {}

    values_t values() const override { return values_; }

    Value value(std::string_view name) const override {
        auto it = values_.find(std::string(name));
        return it == values_.end() ? null : it->second;
    }

 private:
    std::shared_ptr<const Record> cloneImpl() const override {
        return std::make_shared<MockRecord>(*this);
    }

    values_t values_;
};

}  // namespace lsql::exec

#pragma once

#include "data/Log.h"
#include "logs/log_types.h"
#include "rel/Relation.h"

namespace lsql::logs {

class LineRecord : public rel::Record {
 public:
    LineRecord(data::Line line, LogType type) : line_(line) {
        parseKeyValue(line_.view(), type, kv_);
    }

    values_t values() const override {
        values_t values;
        for (auto&& [k, v] : kv_) {
            if (k != "lsql_line") {
                values.emplace(k, std::string(v));
            }
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = kv_.find(name);
        return it == kv_.end() ? Value() : Value(std::string(it->second));
    }

    rel::ConstRecordPtr clone() const override { return std::make_shared<LineRecord>(*this); }

 private:
    void parse() {}

    data::Line line_;
    std::unordered_map<std::string_view, std::string_view> kv_;
};

class LogRelation : public rel::Relation {
 public:
    LogRelation(std::shared_ptr<data::Log> log, LogType type) : log_(std::move(log)), type_(type) {}

    coro::generator<const rel::Record*> records() const override {
        for (auto line : log_->lines()) {
            LineRecord record(line, type_);
            co_yield &record;
        }
    }

 private:
    std::shared_ptr<data::Log> log_;
    LogType type_;
};

}  // namespace lsql::logs

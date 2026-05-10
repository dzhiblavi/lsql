#pragma once

#include "data/Log.h"
#include "exec/op/Source.h"
#include "logs/log_types.h"

namespace lsql::exec {

class LineRecord : public exec::Record {
 public:
    LineRecord(data::Line line, logs::LogType type) : line_(line) {
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

    exec::ConstRecordPtr clone() const override { return std::make_shared<LineRecord>(*this); }

 private:
    data::Line line_;
    std::unordered_map<std::string_view, std::string_view> kv_;
};

class Log : public Source {
 public:
    Log(std::shared_ptr<data::Log> log, logs::LogType type)
        : Source(1, 0)
        , log_(std::move(log))
        , type_(type) {}

    void push(int phase) override {
        if (!active(phase)) {
            return;
        }

        for (auto line : log_->lines()) {
            LineRecord record(line, type_);

            if (!emit(phase, &record)) {
                // no more subscribers
                return;
            }
        }

        emit(phase, nullptr);
    }

 private:
    void init(int) override {}

    std::shared_ptr<data::Log> log_;
    logs::LogType type_;
};

}  // namespace lsql::exec

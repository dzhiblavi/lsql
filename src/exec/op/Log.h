#pragma once

#include "data/Log.h"
#include "exec/Record.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"
#include "logs/log_types.h"

namespace lsql::exec {

class LineRecord : public Record {
 public:
    LineRecord(data::Line line, logs::ParseKeyValueFunc parse_func) : line_(line) {
        parse_func(line.view(), kv_);
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

    ConstRecordPtr cloneImpl() const override { return std::make_shared<LineRecord>(*this); }

 private:
    data::Line line_;
    absl::flat_hash_map<std::string_view, std::string_view> kv_;
};

class Log : public Source, public OperationBase<Log> {
 public:
    Log(std::shared_ptr<data::Log> log, logs::LogType type)
        : OperationBase(0)
        , log_(std::move(log))
        , parse_func_(logs::parseKeyValueFunc(type)) {}

    void push(int phase) override {
        if (!active(phase)) {
            return;
        }

        for (auto line : log_->lines()) {
            LineRecord record(line, parse_func_);

            if (!emit(phase, &record)) {
                // no more subscribers
                return;
            }
        }

        emit(phase, nullptr);
    }

 private:
    // Operation
    void init(int) override {}

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} source: {}", name(), log_->describe());
    }

    std::shared_ptr<data::Log> log_;
    logs::ParseKeyValueFunc parse_func_;
};

}  // namespace lsql::exec

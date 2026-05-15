#pragma once

#include "data/Log.h"
#include "exec/Record.h"
#include "exec/op/OperationBase.h"
#include "exec/op/Source.h"
#include "logs/log_types.h"

namespace lsql::exec {

class LineRecord : public Record {
 public:
    LineRecord(data::Line line, absl::flat_hash_map<std::string_view, std::string_view> values)
        : line_(line)
        , values_(std::move(values)) {}

    values_t values() const override {
        values_t values;
        for (auto&& [k, v] : values_) {
            if (k != "lsql_line") {
                values.emplace(k, std::string(v));
            }
        }
        return values;
    }

    Value value(std::string_view name) const override {
        auto it = values_.find(name);
        return it == values_.end() ? null : Value(std::string(it->second));
    }

    ConstRecordPtr cloneImpl() const override { return std::make_shared<LineRecord>(*this); }

 private:
    data::Line line_;
    absl::flat_hash_map<std::string_view, std::string_view> values_;
};

class Log : public Source, public OperationBase<Log> {
 public:
    Log(std::shared_ptr<data::Log> log, logs::LogType type)
        : OperationBase(0)
        , log_(std::move(log))
        , type_(type) {}

    void push(int phase) override {
        if (!active(phase)) {
            return;
        }

        auto&& required_fields = requiredFields(phase);

        if (required_fields.empty()) {
            auto* record = EmptyRecord::instance().get();

            for (auto _ : log_->lines()) {
                if (!emit(phase, record)) {
                    return;
                }
            }
        } else if (required_fields.all()) {
            absl::flat_hash_map<std::string_view, std::string_view> values;

            auto parser = [&](std::string_view name, std::string_view value) {
                values.emplace(name, value);
            };
            auto parse_func = logs::parseKeyValueFunc<decltype(parser)&>(type_);

            for (auto line : log_->lines()) {
                values.reserve(logs::ExpectedKeysCountTunable);
                parse_func(line.view(), parser);
                LineRecord record(line, std::move(values));
                values = {};

                if (!emit(phase, &record)) {
                    return;
                }
            }
        } else {
            absl::flat_hash_map<std::string_view, std::string_view> values;

            auto parser = [&](std::string_view name, std::string_view value) {
                if (!required_fields.requiresField(name)) {
                    return;
                }
                values.emplace(name, value);
            };

            auto parse_func = logs::parseKeyValueFunc<decltype(parser)&>(type_);

            for (auto line : log_->lines()) {
                values.reserve(required_fields.names().size());
                parse_func(line.view(), parser);
                LineRecord record(line, std::move(values));
                values = {};

                if (!emit(phase, &record)) {
                    return;
                }
            }
        }

        emit(phase, nullptr);
    }

 private:
    // Operation
    void init(int, const RequiredFields&) override {}

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} source: {}", description(ctx.phase), log_->describe());
    }

    std::shared_ptr<data::Log> log_;
    logs::LogType type_;
};

}  // namespace lsql::exec

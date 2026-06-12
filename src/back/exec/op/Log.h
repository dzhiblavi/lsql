#pragma once

#include "back/exec/Record.h"
#include "back/exec/op/OperationBase.h"
#include "back/exec/op/Source.h"
#include "back/logfmt/log_types.h"
#include "back/storage/LineSource.h"

#include "core/PinnedString.h"

namespace lsql::back::exec {

class LineRecord : public Record {
 public:
    LineRecord(absl::flat_hash_map<FieldId, Value> values) : values_(std::move(values)) {}

    const Value& value(FieldId id) const override {
        auto it = values_.find(id);
        return it == values_.end() ? vnull : it->second;
    }

    ConstRecordPtr cloneImpl() const override { return arc<LineRecord>(*this); }

 private:
    absl::flat_hash_map<FieldId, Value> values_;
};

class Log : public Source, public OperationBase<Log> {
    inline static constexpr std::string_view LineIdentifierName = "_line";

 public:
    Log(Arc<back::storage::LineSource> log,
        back::logfmt::LogType type,
        ConstFieldBindingPtr binding)
        : OperationBase(0, std::move(binding))
        , log_(std::move(log))
        , type_(type) {
        prof::addEdge(parse_scope_, prof_);
        prof::addEdge(source_read_scope_, prof_);
    }

    void push(int phase) override {
        if (!active(phase)) {
            return;
        }

        auto&& required_fields = requiredFields(phase).fieldIds();

        if (required_fields.empty()) {
            auto* record = EmptyRecord::instance().get();

            auto lines = log_->lines();
            for (auto it = lines.begin(); it != lines.end(); /* in body */) {
                if (!emit(phase, record)) {
                    return;
                }
                {
                    auto _ = source_read_scope_.scope();
                    ++it;
                }
            }
        } else {
            const size_t max_small_string_size = std::string().capacity();
            absl::flat_hash_map<FieldId, Value> values;
            back::storage::Line line;

            auto insert = [&](FieldId id, std::string_view view) {
                if (view.size() <= max_small_string_size) {
                    values.emplace(id, std::string(view));
                } else {
                    values.emplace(id, PinnedString(line.pin(), view));
                }
            };

            auto parser = [&](std::string_view name, std::string_view value) {
                auto id = binding_->id(name, ValueType::String);

                if (required_fields.contains(id)) {
                    insert(id, value);
                }
            };

            auto parse_func = back::logfmt::parseKeyValueFunc<decltype(parser)&>(type_);
            const auto line_id = binding_->id(LineIdentifierName, ValueType::String);
            const bool has_line = required_fields.contains(line_id);
            const size_t values_count = required_fields.size() + (has_line ? 1 : 0);

            auto lines = log_->lines();
            for (auto it = lines.begin(); it != lines.end(); /* in body */) {
                line = *it;
                {
                    auto _ = parse_scope_.scope();

                    values.reserve(values_count);
                    parse_func(line.view(), parser);
                    if (has_line) {
                        insert(line_id, line.view());
                    }
                }

                LineRecord record(std::move(values));
                values = {};

                if (!emit(phase, &record)) {
                    return;
                }

                {
                    auto _ = source_read_scope_.scope();
                    ++it;
                }
            }
        }

        emit(phase, nullptr);
    }

 private:
    // Operation
    void init(int, const FieldSet&) override {}

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} source: {}", description(ctx.phase), log_->describe());
    }

    Arc<back::storage::LineSource> log_;
    back::logfmt::LogType type_;

    prof::ScopeHandle<prof::ScopeMetrics<>> source_read_scope_ =
        prof::newScope<prof::ScopeMetrics<>>("read: {}", log_->describe());
    prof::ScopeHandle<prof::ScopeMetrics<>> parse_scope_ =
        prof::newScope<prof::ScopeMetrics<>>(std::string("parse"));
};

}  // namespace lsql::back::exec

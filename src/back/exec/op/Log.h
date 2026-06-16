#pragma once

#include "back/exec/Record.h"
#include "back/exec/op/OperationBase.h"
#include "back/exec/op/Source.h"
#include "back/logfmt/log_types.h"
#include "back/storage/LineSource.h"

#include "core/schema/Schema.h"

#include "config/build_settings.h"

namespace lsql::back::exec {

class Log : public Source, public OperationBase<Log> {
 public:
    Log(Arc<back::storage::LineSource> log,
        back::logfmt::LogType type,
        Schema schema,
        ConstFieldBindingPtr binding)
        : OperationBase(0, std::move(binding))
        , log_(std::move(log))
        , schema_(std::move(schema))
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
            prepareSlots(phase);

            const size_t max_small_string_size = std::string().capacity();
            std::vector<Value> values;
            back::storage::Line line;

            auto insert = [&](SlotId slot, std::string_view view) {
                verify_dbg(0 <= slot && slot < values.size());

                if (view.size() <= max_small_string_size) {
                    values[slot] = std::string(view);
                } else {
                    values[slot] = line.subview(view);
                }
            };

            auto parser = [&](std::string_view name, std::string_view value) {
                if (auto it = slots_.find(name); it != slots_.end()) {
                    insert(it->second, value);
                }
            };

            auto parse_func = back::logfmt::parseKeyValueFunc<decltype(parser)&>(type_);
            const auto line_slot = slots_.find(config::Semantics::LineIdentifier);
            const bool has_line = line_slot != slots_.end();

            auto lines = log_->lines();
            for (auto it = lines.begin(); it != lines.end(); /* in body */) {
                values.assign(schema_.columns(), vnull);
                line = *it;

                {
                    auto _ = parse_scope_.scope();
                    parse_func(line.view(), parser);
                    if (has_line) {
                        insert(line_slot->second, line.view());
                    }
                }

                VecRecord record(std::move(values));
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

    void prepareSlots(int phase) {
        slots_.clear();

        for (auto id : requiredFields(phase).fieldIds()) {
            if (auto slot = schema_.slot(id); slot.has_value()) {
                slots_.emplace(binding_->name(id), *slot);
            }
        }
    }

    // Operation
    ExplanationItem explain(ExplanationCtx ctx) const override {
        if (!hasSubscriber(ctx.phase, ctx.requester)) {
            return {};
        }

        return ExplanationItem().line("{} source: {}", description(ctx.phase), log_->describe());
    }

    Arc<back::storage::LineSource> log_;
    Schema schema_;
    back::logfmt::LogType type_;

    absl::flat_hash_map<std::string_view, SlotId> slots_;

    prof::ScopeHandle<prof::ScopeMetrics<>> source_read_scope_ =
        prof::newScope<prof::ScopeMetrics<>>("read: {}", log_->describe());
    prof::ScopeHandle<prof::ScopeMetrics<>> parse_scope_ =
        prof::newScope<prof::ScopeMetrics<>>(std::string("parse"));
};

}  // namespace lsql::back::exec

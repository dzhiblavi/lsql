#pragma once

#include "back/exec/Record.h"
#include "back/exec/phys/Operation.h"
#include "back/exec/phys/Source.h"
#include "back/logfmt/log_types.h"
#include "back/storage/LineSource.h"

#include "config/build_settings.h"

namespace lsql::back::exec::phys {

class Log : public Source, public OperationBase<Log> {
 public:
    Log(int id,
        Arc<back::storage::LineSource> log,
        back::logfmt::LogType type,
        absl::flat_hash_map<std::string_view, SlotId> slots,
        uint32_t num_slots)
        : OperationBase(id)
        , log_(std::move(log))
        , type_(type)
        , slots_(std::move(slots))
        , num_slots_(num_slots) {
        prof::addEdge(parse_scope_, prof_);
        prof::addEdge(source_read_scope_, prof_);
    }

    void push() override {
        verify(active());

        if (slots_.empty()) {
            auto* record = EmptyRecord::instance().get();

            auto lines = log_->lines();
            for (auto it = lines.begin(); it != lines.end(); /* in body */) {
                if (!emit(record)) {
                    return;
                }
                {
                    auto _ = source_read_scope_.scope();
                    ++it;
                }
            }
        } else {
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
                values.assign(num_slots_, vnull);
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

                if (!emit(&record)) {
                    return;
                }

                {
                    auto _ = source_read_scope_.scope();
                    ++it;
                }
            }
        }

        emit(nullptr);
    }

 private:
    Arc<back::storage::LineSource> log_;
    back::logfmt::LogType type_;
    absl::flat_hash_map<std::string_view, SlotId> slots_;
    uint32_t num_slots_;

    prof::ScopeHandle<prof::ScopeMetrics<>> source_read_scope_ =
        prof::newScope<prof::ScopeMetrics<>>("read: {}", log_->describe());
    prof::ScopeHandle<prof::ScopeMetrics<>> parse_scope_ =
        prof::newScope<prof::ScopeMetrics<>>(std::string("parse"));
};

}  // namespace lsql::back::exec::phys

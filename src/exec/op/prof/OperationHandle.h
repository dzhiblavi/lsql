#pragma once

#include "util/instrument/Counters.h"
#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"

#include "exec/op/prof/InputHandle.h"
#include "exec/op/prof/detail/ThreadStats.h"

namespace lsql::exec::prof {

class OperationHandle {
    struct EmitScope {
        EmitScope() = default;

        EmitScope(
            detail::ThreadOperationStats* stats,
            OperationHandle* self,
            InputHandle::ConsumeScope** parent)
            : started_at(instr::MonotonicClock::now())
            , stats(stats)
            , self(self)
            , parent(parent) {
            ++stats->records_out;
        }

        ~EmitScope() {
            if (!parent) {
                return;
            }

            auto dur = instr::MonotonicClock::now() - started_at;
            if (*parent) {
                (*parent)->ignore_duration += dur;
            }
            stats->emit_profile.add(dur);
        }

        instr::MonotonicTimePoint started_at = {};
        detail::ThreadOperationStats* stats = nullptr;
        OperationHandle* self = nullptr;
        InputHandle::ConsumeScope** parent = nullptr;
    };

 public:
    OperationHandle() = default;
    explicit OperationHandle(detail::OperationStats* stats) : stats_(stats) {}

    EmitScope emitScope() {
        if (!stats_) {
            return {};
        }
        return {stats_->current(), this, &consume_scope};
    }

    InputHandle inputHandle(const Subscriber* sub) {
        if (!stats_) {
            return {};
        }
        return {stats_->input(sub), &consume_scope};
    }

    detail::ThreadOperationStats* current() {
        if (!stats_) {
            return nullptr;
        }
        return stats_->current();
    }

    instr::Counter<int64_t>* makeCounter(std::string_view name, int64_t init = 0) {
        if (!stats_) {
            return nullptr;
        }
        return &stats_->makeCounter(name, init).counter;
    }

 private:
    detail::OperationStats* stats_ = nullptr;
    InputHandle::ConsumeScope* consume_scope = nullptr;
};

}  // namespace lsql::exec::prof

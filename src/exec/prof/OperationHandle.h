#pragma once

#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"

#include "exec/prof/InputHandle.h"
#include "exec/prof/detail/ThreadStats.h"

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
        return {stats_->currentThread(), this, &consume_scope};
    }

    InputHandle inputHandle(const Subscriber* sub) {
        if (!stats_) {
            return {};
        }
        return {stats_->input(sub), &consume_scope};
    }

    detail::ThreadOperationStats* currentThread() {
        if (!stats_) {
            return nullptr;
        }
        return stats_->currentThread();
    }

    void registerMetric(Metric* metric) {
        if (!stats_) {
            return;
        }
        stats_->registerMetric(metric);
    }

    operator bool() const { return stats_ != nullptr; }

 private:
    detail::OperationStats* stats_ = nullptr;
    InputHandle::ConsumeScope* consume_scope = nullptr;
};

OperationHandle& currentOperation();

}  // namespace lsql::exec::prof

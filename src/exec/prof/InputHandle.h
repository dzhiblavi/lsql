#pragma once

#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"

#include "exec/prof/detail/Stats.h"
#include "exec/prof/detail/ThreadStats.h"

namespace lsql::exec::prof {

class InputHandle {
    friend class OperationHandle;

    struct ConsumeScope {
        ConsumeScope() = default;

        ConsumeScope(detail::ThreadSubscriberStats* stats, InputHandle* self)
            : started_at(instr::MonotonicClock::now())
            , stats_(stats)
            , self(self) {
            *self->current_ = this;
            ++stats->records_in;
        }

        ~ConsumeScope() {
            if (!self) {
                return;
            }

            *self->current_ = nullptr;
            stats_->consume_profile.add(
                instr::MonotonicClock::now() - started_at - ignore_duration);
        }

        instr::MonotonicTimePoint started_at = {};
        instr::MonotonicDuration ignore_duration = {};
        detail::ThreadSubscriberStats* stats_ = nullptr;
        InputHandle* self = nullptr;
    };

 public:
    InputHandle() = default;

    InputHandle(detail::Stats<detail::ThreadSubscriberStats>* stats, ConsumeScope** current)
        : current_(current)
        , stats_(stats) {}

    ConsumeScope consumeScope() {
        if (!current_) {
            return {};
        }
        return {stats_->current(), this};
    }

 private:
    ConsumeScope** current_ = nullptr;
    detail::Stats<detail::ThreadSubscriberStats>* stats_ = nullptr;
};

}  // namespace lsql::exec::prof

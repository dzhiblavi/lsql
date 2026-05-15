#pragma once

#include "util/NonCopyable.h"
#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"

#include "exec/prof/detail/Stats.h"
#include "exec/prof/detail/ThreadStats.h"

namespace lsql::exec::prof {

class OperationHandle;

void pushCurrentOperation(OperationHandle* handle);
void popCurrentOperation(OperationHandle* handle);
OperationHandle& currentOperation();

class InputHandle {
    friend class OperationHandle;

    struct ConsumeScope : util::NonCopyable {
        ConsumeScope() = default;

        ConsumeScope(ConsumeScope&& rhs) noexcept
            : started_at(rhs.started_at)
            , ignore_duration(rhs.ignore_duration)
            , stats_(std::exchange(rhs.stats_, nullptr))
            , self(std::exchange(rhs.self, nullptr)) {
            if (self) {
                *self->current_ = this;
            }
        }

        ConsumeScope(detail::ThreadSubscriberStats* stats, InputHandle* self)
            : started_at(instr::MonotonicClock::now())
            , stats_(stats)
            , self(self) {
            *self->current_ = this;
            ++stats->records_in;
            pushCurrentOperation(self->operation_);
        }

        ~ConsumeScope() {
            if (!self) {
                return;
            }

            verify(*self->current_ == this);
            *self->current_ = nullptr;
            stats_->consume_profile.add(
                instr::MonotonicClock::now() - started_at - ignore_duration);

            popCurrentOperation(self->operation_);
        }

        instr::MonotonicTimePoint started_at = {};
        instr::MonotonicDuration ignore_duration = {};
        detail::ThreadSubscriberStats* stats_ = nullptr;
        InputHandle* self = nullptr;
    };

 public:
    InputHandle() = default;

    InputHandle(
        detail::Stats<detail::ThreadSubscriberStats>* stats,
        OperationHandle* operation,
        ConsumeScope** current)
        : current_(current)
        , operation_(operation)
        , stats_(stats) {
        verify(operation_ != nullptr);
    }

    ConsumeScope consumeScope() {
        if (!current_) {
            return {};
        }
        return {stats_->current(), this};
    }

 private:
    ConsumeScope** current_ = nullptr;
    OperationHandle* operation_ = nullptr;
    detail::Stats<detail::ThreadSubscriberStats>* stats_ = nullptr;
};

}  // namespace lsql::exec::prof

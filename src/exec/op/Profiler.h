#pragma once

#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"

#include <unordered_map>

namespace lsql::exec {

class Operation;
class Subscriber;

struct SubscriberStats {
    uint32_t records_in = 0;
    instr::SequenceProfile<std::chrono::microseconds> consume_profile = {};

    void reset() {
        records_in = 0;
        consume_profile.reset();
    }
};

struct OperationStats {
    std::string_view name;
    uint32_t records_out = 0;
    instr::SequenceProfile<std::chrono::microseconds> emit_profile = {};
    std::unordered_map<const Subscriber*, SubscriberStats> inputs = {};

    void reset() {
        records_out = 0;
        emit_profile.reset();

        for (auto&& [_, input] : inputs) {
            input.reset();
        }
    }
};

class Profiler {
 public:
    struct InputHandle {
        struct ConsumeScope {
            ConsumeScope(InputHandle* self) : self(self) { self->enterConsume(this); }
            ~ConsumeScope() {
                self->exitConsume(instr::MonotonicClock::now() - started_at - ignore_duration);
            }

            instr::MonotonicTimePoint started_at = instr::MonotonicClock::now();
            instr::MonotonicDuration ignore_duration = {};
            InputHandle* self;
        };

        InputHandle(SubscriberStats* stats, ConsumeScope** current)
            : current_(current)
            , stats_(stats) {}

        ConsumeScope consumeScope() { return {this}; }

     private:
        void enterConsume(ConsumeScope* scope) {
            *current_ = scope;
            ++stats_->records_in;
        }

        void exitConsume(instr::MonotonicDuration dur) { stats_->consume_profile.add(dur); }

        ConsumeScope** current_;
        SubscriberStats* stats_;
    };

    struct OperationHandle {
        struct EmitScope {
            EmitScope(OperationHandle* self, InputHandle::ConsumeScope** parent)
                : parent(parent)
                , self(self) {
                self->enterEmit();
            }

            ~EmitScope() {
                auto dur = instr::MonotonicClock::now() - started_at;
                if (*parent) {
                    (*parent)->ignore_duration += dur;
                }
                self->exitEmit(dur);
            }

            instr::MonotonicTimePoint started_at = instr::MonotonicClock::now();
            InputHandle::ConsumeScope** parent;  // emit may be called from inside consume
            OperationHandle* self;
        };

        explicit OperationHandle(OperationStats* stats) : stats_(stats) {}

        EmitScope emitScope() { return {this, &consume_scope}; }

        InputHandle inputHandle(const Subscriber* sub) {
            return InputHandle(&stats_->inputs[sub], &consume_scope);
        }

     private:
        void enterEmit() { ++stats_->records_out; }
        void exitEmit(instr::MonotonicDuration dur) { stats_->emit_profile.add(dur); }

        OperationStats* stats_;
        InputHandle::ConsumeScope* consume_scope = nullptr;
    };

    OperationHandle registerOperation(const Operation* self, std::string_view name);
    std::string report() const;
    void reset();

    static Profiler& profiler();

 private:
    Profiler() = default;

    std::unordered_map<const Operation*, OperationStats> stats_;
};

}  // namespace lsql::exec

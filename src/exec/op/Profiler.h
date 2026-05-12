#pragma once

#include "core/verify.h"
#include "util/instrument/SequenceProfile.h"
#include "util/instrument/types.h"
#include "util/thread_name.h"

#include <unordered_map>
#include <vector>

namespace lsql::exec {

class Operation;
class Subscriber;

struct ThreadSubscriberStats {
    uint32_t records_in = 0;
    instr::SequenceProfile<std::chrono::microseconds> consume_profile = {};

    void reset() {
        records_in = 0;
        consume_profile.reset();
    }

    bool empty() const { return records_in == 0; }
};

struct ThreadOperationStats {
    uint32_t records_out = 0;
    instr::SequenceProfile<std::chrono::microseconds> emit_profile = {};
    std::vector<std::string> custom_info;

    void reset() {
        records_out = 0;
        emit_profile.reset();
    }

    template <typename... Args>
    void custom(std::format_string<const Args&...> fmt, const Args&... args) {
        this->custom_info.push_back(std::format(fmt, args...));
    }

    bool empty() const { return records_out == 0 && custom_info.empty(); }
};

template <typename T>
struct Stats {
    explicit Stats(size_t num_threads) : threads_(num_threads) {}

    T* thread(size_t index) {
        verify(index < threads_.size());
        return &threads_[index];
    }

    T* current() { return thread(util::threadIndex()); }
    void reset() { std::ranges::for_each(threads_, &T::reset); }

 private:
    std::vector<T> threads_;
};

struct OperationStats {
    OperationStats(size_t num_threads, std::string_view name)
        : num_threads_(num_threads)
        , name_(name)
        , op_(num_threads) {}

    ThreadOperationStats* thread(size_t index) { return op_.thread(index); }
    ThreadOperationStats* current() { return op_.current(); }

    Stats<ThreadSubscriberStats>* input(const Subscriber* sub) {
        auto it = inputs_.find(sub);
        if (it == inputs_.end()) {
            it = inputs_.emplace(sub, num_threads_).first;
        }
        return &it->second;
    }

    auto& inputs() { return inputs_; }

    bool empty(size_t index) {
        if (!thread(index)->empty()) {
            return false;
        }
        for (auto&& [_, v] : inputs_) {
            if (!v.thread(index)->empty()) {
                return false;
            }
        }
        return true;
    }

    void reset() {
        op_.reset();
        for (auto&& [k, v] : inputs_) {
            v.reset();
        }
    }

    std::string_view name() const { return name_; }

 private:
    template <typename T>
    using ThreadVec = std::vector<std::unique_ptr<T>>;

    size_t num_threads_;
    std::string_view name_;
    Stats<ThreadOperationStats> op_;
    std::unordered_map<const Subscriber*, Stats<ThreadSubscriberStats>> inputs_;
};

struct InputHandle {
    struct ConsumeScope {
        ConsumeScope(ThreadSubscriberStats* stats, InputHandle* self) : stats_(stats), self(self) {
            *self->current_ = this;
            ++stats->records_in;
        }

        ~ConsumeScope() {
            *self->current_ = nullptr;
            stats_->consume_profile.add(
                instr::MonotonicClock::now() - started_at - ignore_duration);
        }

        instr::MonotonicTimePoint started_at = instr::MonotonicClock::now();
        instr::MonotonicDuration ignore_duration = {};
        ThreadSubscriberStats* stats_;
        InputHandle* self;
    };

    InputHandle(Stats<ThreadSubscriberStats>* stats, ConsumeScope** current)
        : current_(current)
        , stats_(stats) {}

    ConsumeScope consumeScope() { return {stats_->current(), this}; }

 private:
    ConsumeScope** current_;
    Stats<ThreadSubscriberStats>* stats_;
};

struct OperationHandle {
    struct EmitScope {
        EmitScope(
            ThreadOperationStats* stats, OperationHandle* self, InputHandle::ConsumeScope** parent)
            : stats(stats)
            , self(self)
            , parent(parent) {
            ++stats->records_out;
        }

        ~EmitScope() {
            auto dur = instr::MonotonicClock::now() - started_at;
            if (*parent) {
                (*parent)->ignore_duration += dur;
            }
            stats->emit_profile.add(dur);
        }

        instr::MonotonicTimePoint started_at = instr::MonotonicClock::now();
        ThreadOperationStats* stats;
        OperationHandle* self;
        InputHandle::ConsumeScope** parent;  // emit may be called from inside consume
    };

    explicit OperationHandle(OperationStats* stats) : stats_(stats) {}

    EmitScope emitScope() { return {stats_->current(), this, &consume_scope}; }
    InputHandle inputHandle(const Subscriber* sub) { return {stats_->input(sub), &consume_scope}; }
    ThreadOperationStats& current() { return *stats_->current(); }

 private:
    OperationStats* stats_;
    InputHandle::ConsumeScope* consume_scope = nullptr;
};

class Profiler {
 public:
    Profiler(size_t num_threads);
    ~Profiler();

    OperationHandle registerOperation(const Operation* self, std::string_view name);
    std::string report();
    void reset();

    static Profiler& profiler();

 private:
    const size_t num_threads_;
    std::unordered_map<const Operation*, OperationStats> stats_;
};

}  // namespace lsql::exec

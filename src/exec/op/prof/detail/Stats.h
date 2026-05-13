#pragma once

#include "exec/op/prof/detail/NamedCounter.h"

#include "core/verify.h"
#include "util/thread_name.h"

#include "exec/op/prof/detail/ThreadStats.h"

#include <list>
#include <unordered_map>
#include <vector>

namespace lsql::exec {

class Operation;
class Subscriber;

}  // namespace lsql::exec

namespace lsql::exec::prof::detail {

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
    explicit OperationStats(size_t num_threads) : num_threads_(num_threads), op_(num_threads) {}

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

        std::ranges::for_each(counters_, &NamedCounter::reset);
    }

    NamedCounter& makeCounter(std::string_view name, int64_t init) {
        return counters_.emplace_back(name, init);
    }

    const std::list<NamedCounter>& counters() const { return counters_; }

 private:
    template <typename T>
    using ThreadVec = std::vector<std::unique_ptr<T>>;

    size_t num_threads_;
    Stats<ThreadOperationStats> op_;
    std::unordered_map<const Subscriber*, Stats<ThreadSubscriberStats>> inputs_;
    std::list<NamedCounter> counters_;
};

}  // namespace lsql::exec::prof::detail

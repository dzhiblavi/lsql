#pragma once

#include "core/verify.h"
#include "util/thread_name.h"

#include <cassert>
#include <functional>
#include <queue>
#include <thread>

namespace lsql::util {

template <typename T>
class MPMCLockedQueue {
 public:
    MPMCLockedQueue() = default;

    std::optional<T> pop() {
        std::unique_lock lg(m_);

        if (!queue_.empty()) {
            return popQueue();
        }

        cv_.wait(lg, [&] { return !queue_.empty() || stopped_.load(std::memory_order_relaxed); });

        if (stopped_.load(std::memory_order_relaxed)) {
            return std::nullopt;
        }

        verify_dbg(!queue_.empty());
        return popQueue();
    }

    void push(T value) {
        {
            std::unique_lock lg(m_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    void stop() {
        stopped_.store(true, std::memory_order_relaxed);
        cv_.notify_all();
    }

 private:
    T popQueue() {
        assert(!queue_.empty());
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    std::atomic<bool> stopped_ = false;
    std::queue<T> queue_;
    std::mutex m_;
    std::condition_variable cv_;
};

class ThreadPool {
 public:
    explicit ThreadPool(size_t threads) {
        workers_.reserve(threads);

        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back(&ThreadPool::worker, this, i);
        }
    }

    void enqueue(std::function<void()> task) { tasks_.push(std::move(task)); }

    void stop() { tasks_.stop(); }

    size_t size() const { return workers_.size(); }

    void join() {
        for (auto& thread : workers_) {
            if (!thread.joinable()) {
                continue;
            }

            thread.join();
        }
    }

 private:
    void worker(size_t index) {
        setThreadName(std::format("worker-{}", index));
        llog::info("started worker thread [thread={}]", threadName());

        for (;;) {
            auto task = tasks_.pop();

            if (!task.has_value()) {
                // stopped
                break;
            }

            try {
                (*task)();
            } catch (...) {
                panic("unhandled exception in ThreadPool task");
            }
        }

        llog::info("stopping worker thread [thread={}]", threadName());
        setThreadName("");
    }

    std::vector<std::thread> workers_;
    MPMCLockedQueue<std::function<void()>> tasks_;
};

}  // namespace lsql::util

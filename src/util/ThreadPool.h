#pragma once

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

        assert(!queue_.empty());
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
            workers_.emplace_back(&ThreadPool::worker, this);
        }
    }

    void enqueue(std::function<void()> task) { tasks_.push(std::move(task)); }

    void stop() { tasks_.stop(); }

    void join() {
        for (auto& thread : workers_) {
            if (!thread.joinable()) {
                continue;
            }

            thread.join();
        }
    }

 private:
    void worker() {
        for (;;) {
            auto task = tasks_.pop();

            if (!task.has_value()) {
                // stopped
                break;
            }

            (*task)();
        }
    }

    std::vector<std::thread> workers_;
    MPMCLockedQueue<std::function<void()>> tasks_;
};

}  // namespace lsql::util

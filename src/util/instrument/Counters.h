#pragma once

#include <algorithm>
#include <atomic>
#include <concepts>

namespace lsql::instr {

template <std::integral T>
class Counter {
 public:
    Counter() = default;
    explicit Counter(T init) : value_{init} {}
    Counter(const Counter& r) : value_(r.value()) {}

    T value() const { return value_.load(std::memory_order_relaxed); }
    void set(T value) { value_.store(value, std::memory_order_relaxed); }
    void add(T value) { value_.fetch_add(value, std::memory_order_relaxed); }
    void sub(T value) { value_.fetch_sub(value, std::memory_order_relaxed); }

    void max(T value) {
        T expected = this->value();
        T new_value;
        do {
            new_value = std::max(expected, value);
        } while (!value_.compare_exchange_weak(
            expected, new_value, std::memory_order_relaxed, std::memory_order_relaxed));
    }

    void min(T value) {
        T expected = this->value();
        T new_value;
        do {
            new_value = std::min(expected, value);
        } while (!value_.compare_exchange_weak(
            expected, new_value, std::memory_order_relaxed, std::memory_order_relaxed));
    }

 private:
    std::atomic<T> value_{0};
};

}  // namespace lsql::instr

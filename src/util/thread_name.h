#pragma once

#include <atomic>
#include <string>

namespace lsql::util {

namespace detail {

inline thread_local std::string thread_name_;

inline size_t nextThreadIndex() {
    static std::atomic<size_t> next{0};
    return next.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace detail

inline void setThreadName(std::string name) {
    detail::thread_name_ = std::move(name);
}

inline std::string_view threadName() {
    return detail::thread_name_;
}

inline size_t threadIndex() {
    thread_local const size_t id = detail::nextThreadIndex();
    return id;
}

}  // namespace lsql::util

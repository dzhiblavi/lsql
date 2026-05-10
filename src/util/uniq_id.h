#pragma once

#include <atomic>

namespace lsql::util {

inline int uniqId() {
    static std::atomic<int> next;
    return next.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace lsql::util

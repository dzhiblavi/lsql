#include "util/uniq_id.h"

#include <atomic>

namespace lsql::util {

int uniqId() {
    static std::atomic<int> next;
    return next.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace lsql::util

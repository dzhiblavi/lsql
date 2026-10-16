#include "util/PageSize.h"

#include <unistd.h>

namespace lsql::util {

size_t systemPageSize() {
    static const size_t page_size = sysconf(_SC_PAGE_SIZE);
    return page_size;
}

size_t pageSize(size_t multiplier) {
    return multiplier * systemPageSize();
}

size_t pageCount(size_t file_size, size_t page_size_multiplier) {
    const auto page_size = pageSize(page_size_multiplier);
    size_t count = file_size / page_size;
    size_t remainder = file_size % page_size;
    return remainder == 0 ? count : count + 1;
}

}  // namespace lsql::util

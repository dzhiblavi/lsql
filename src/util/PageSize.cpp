#include "util/PageSize.h"
#include "config/build_settings.h"

#include <unistd.h>

namespace lsql::util {

namespace {

const size_t page_size_ = sysconf(_SC_PAGE_SIZE);

}  // namespace

size_t systemPageSize() {
    return page_size_;
}

size_t pageSize() {
    return config::Storage::PageSizeMultiplier * systemPageSize();
}

size_t pageCount(size_t file_size) {
    size_t count = file_size / util::pageSize();
    size_t remainder = file_size % util::pageSize();
    return remainder == 0 ? count : count + 1;
}

}  // namespace lsql::util

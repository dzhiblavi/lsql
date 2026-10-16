#pragma once

#include <cstddef>

namespace lsql::util {

size_t pageCount(size_t file_size, size_t page_size_multiplier);
size_t pageSize(size_t multiplier);

}  // namespace lsql::util

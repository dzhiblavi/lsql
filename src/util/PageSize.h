#pragma once

#include <cstddef>

namespace lsql::util {

size_t pageCount(size_t file_size);
size_t pageSize();

}  // namespace lsql::util

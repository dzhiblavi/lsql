#pragma once

#include <string_view>

namespace lsql::util {

std::string_view suffix(std::string_view s, size_t max_size);

}  // namespace lsql::util

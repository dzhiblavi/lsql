#pragma once

#include <string_view>

namespace lsql::util {

bool isProbablyArchive(std::string_view path);

}  // namespace lsql::util

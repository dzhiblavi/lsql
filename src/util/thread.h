#pragma once

#include <string>

namespace lsql::util {

void setThreadName(std::string name);
std::string_view threadName();

size_t threadIndex();

}  // namespace lsql::util

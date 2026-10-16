#pragma once

#include <ostream>
#include <span>

namespace lsql {

void parseSettings(std::span<const std::string> options);

void dumpSettings(std::ostream& os, std::string_view prefix = "");

}  // namespace lsql

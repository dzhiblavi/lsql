#include "util/string.h"

namespace lsql::util {

std::string_view suffix(std::string_view s, size_t max_size) {
    return s.size() > max_size ? s.substr(s.size() - max_size) : s;
}

}  // namespace lsql::util

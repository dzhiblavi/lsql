#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace lsql::util {

template <typename T>
static std::string toString(const std::vector<T>& values) {
    using std::to_string;

    std::stringstream ss;
    ss << '[';
    for (auto&& v : values) {
        ss << to_string(v) << ',';
    }
    if (!values.empty()) {
        ss.seekp(-1, std::ios_base::end);
    }
    ss << ']';
    return ss.str();
}

}  // namespace lsql::util

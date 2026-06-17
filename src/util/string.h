#pragma once

#include <sstream>
#include <string>
#include <vector>

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

inline std::string stripNamespace(std::string s) {
    auto pos = s.find_last_of(':');
    if (pos == std::string::npos) {
        return s;
    }
    return s.substr(pos + 1);
}

}  // namespace lsql::util

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace lsql::util {

inline std::optional<std::string> jsonPathToPointer(std::string_view path) {
    if (path.empty()) {
        return std::string();
    }

    if (path.starts_with('/')) {
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == '~' &&
                (i + 1 == path.size() || (path[i + 1] != '0' && path[i + 1] != '1'))) {
                return std::nullopt;
            }
        }
        return std::string(path);
    }

    size_t pos = 0;
    if (path[pos] == '$') {
        ++pos;
        if (pos == path.size()) {
            return std::string();
        }
        if (path[pos] == '.') {
            ++pos;
        } else if (path[pos] != '[') {
            return std::nullopt;
        }
    } else if (path[pos] == '.') {
        ++pos;
    }

    if (pos == path.size()) {
        return std::nullopt;
    }

    std::string pointer;
    auto append_segment = [&pointer](std::string_view segment) {
        pointer.push_back('/');
        for (char c : segment) {
            if (c == '~') {
                pointer += "~0";
            } else if (c == '/') {
                pointer += "~1";
            } else {
                pointer.push_back(c);
            }
        }
    };

    while (pos < path.size()) {
        if (path[pos] == '[') {
            const size_t begin = ++pos;
            while (pos < path.size() && path[pos] >= '0' && path[pos] <= '9') {
                ++pos;
            }
            if (begin == pos || pos == path.size() || path[pos] != ']') {
                return std::nullopt;
            }
            append_segment(path.substr(begin, pos - begin));
            ++pos;
        } else {
            const size_t begin = pos;
            while (pos < path.size() && path[pos] != '.' && path[pos] != '[') {
                ++pos;
            }
            if (begin == pos) {
                return std::nullopt;
            }
            append_segment(path.substr(begin, pos - begin));
        }

        if (pos == path.size()) {
            break;
        }
        if (path[pos] == '.') {
            ++pos;
            if (pos == path.size() || path[pos] == '.' || path[pos] == '[') {
                return std::nullopt;
            }
        } else if (path[pos] != '[') {
            return std::nullopt;
        }
    }

    return pointer;
}

}  // namespace lsql::util

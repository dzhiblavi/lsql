#pragma once

#include <format>
#include <sstream>
#include <vector>

namespace lsql::util {

class StrBuilder {
 public:
    StrBuilder() = default;
    StrBuilder(std::string s) : lines_{std::move(s)} {}

    template <typename... Args>
    StrBuilder(std::format_string<const Args&...> fmt, const Args&... args) {
        lines_.push_back(std::format(fmt, args...));
    }

    std::string render() {
        std::stringstream ss;
        for (auto&& line : lines_) {
            ss << line << '\n';
        }
        return ss.str();
    }

    StrBuilder& child(StrBuilder builder) {
        for (auto&& line : builder.lines_) {
            lines_.push_back(std::format("  {}", line));
        }
        return *this;
    }

    template <typename... Args>
    StrBuilder& line(std::format_string<const Args&...> fmt, const Args&... args) {
        lines_.push_back(std::format(fmt, args...));
        return *this;
    }

 private:
    std::vector<std::string> lines_;
};

}  // namespace lsql::util

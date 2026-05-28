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

    bool empty() const { return lines_.empty(); }

    std::string render() const {
        std::stringstream ss;
        for (auto&& line : lines_) {
            ss << line << '\n';
        }
        return ss.str();
    }

    StrBuilder& item(StrBuilder builder) {
        if (builder.lines_.empty()) {
            return *this;
        }

        lines_.push_back(std::format("-  {}", builder.lines_.front()));
        for (size_t i = 1; i < builder.lines_.size(); ++i) {
            lines_.push_back(std::format("|  {}", builder.lines_[i]));
        }
        return *this;
    }

    template <typename... Args>
    StrBuilder& item(std::format_string<const Args&...> fmt, const Args&... args) {
        return item(StrBuilder(fmt, args...));
    }

    StrBuilder& child(StrBuilder builder) {
        for (auto&& line : builder.lines_) {
            lines_.push_back(std::format("|  {}", line));
        }
        return *this;
    }

    StrBuilder& block(StrBuilder builder) {
        for (auto&& line : builder.lines_) {
            lines_.push_back(std::move(line));
        }
        return *this;
    }

    StrBuilder& line(std::string s) {
        lines_.push_back(std::move(s));
        return *this;
    }

    template <typename... Args>
    requires(sizeof...(Args) > 0)
    StrBuilder& line(std::format_string<const Args&...> fmt, const Args&... args) {
        lines_.push_back(std::format(fmt, args...));
        return *this;
    }

 private:
    std::vector<std::string> lines_;
};

}  // namespace lsql::util

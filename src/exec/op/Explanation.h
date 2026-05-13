#pragma once

#include "exec/op/Subscriber.h"

#include <format>
#include <sstream>

namespace lsql::exec {

class Operation;

class ExplanationItem {
 public:
    ExplanationItem() = default;

    bool empty() const { return lines.empty(); }

    ExplanationItem& line(std::string s) {
        lines.push_back(std::move(s));
        return *this;
    }

    template <typename... Args>
    ExplanationItem& line(std::format_string<const Args&...> fmt, const Args&... args) {
        lines.push_back(std::format(fmt, args...));
        return *this;
    }

    ExplanationItem& child(ExplanationItem explanation) {
        for (auto&& line : explanation.lines) {
            lines.push_back(std::format("  {}", line));
        }
        return *this;
    }

    ExplanationItem& block(ExplanationItem explanation) {
        for (auto&& line : explanation.lines) {
            lines.push_back(std::move(line));
        }
        return *this;
    }

    std::string format() const {
        std::stringstream ss;
        for (auto&& line : lines) {
            ss << line << '\n';
        }
        return ss.str();
    }

 private:
    std::vector<std::string> lines;
};

class Explanation {
 public:
    Explanation() = default;

    void insert(ExplanationItem item, const Operation* op) {
        if (item.empty()) {
            return;
        }

        items.emplace(op, std::move(item));
    }

    std::string format() const {
        std::stringstream ss;
        for (auto&& [_, item] : items) {
            ss << item.format() << '\n';
        }
        return ss.str();
    }

 private:
    std::unordered_map<const Operation*, ExplanationItem> items;
};

struct ExplanationCtx {
    const Subscriber* requester = nullptr;
    const int phase = 0;
    Explanation& explanation;

    ExplanationCtx withRequester(const Subscriber* sub) const {
        ExplanationCtx ctx{*this};
        ctx.requester = sub;
        return ctx;
    }
};

}  // namespace lsql::exec

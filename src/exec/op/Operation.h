#pragma once

#include "core/verify.h"
#include "exec/op/Subscriber.h"

#include <memory>
#include <sstream>
#include <unordered_set>

namespace lsql::exec {

class Operation;

class ExplanationItem {
 public:
    ExplanationItem() = default;

    bool empty() const { return lines.empty(); }

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

class Operation {
    using Subscribers = std::unordered_map<int, std::unordered_set<Subscriber*>>;

 public:
    Operation(int phases, int min_out_phase) : phases_(phases), min_out_phase_(min_out_phase) {}

    virtual ~Operation() = default;

    // outbound operations call this method to receive records
    // idempotent
    void subscribe(int out_phase, Subscriber* sub) {
        verify(out_phase >= minPhase());

        if (subs_[out_phase].contains(sub)) {
            return;
        }

        max_out_phase_ = std::max(max_out_phase_, out_phase);
        subs_[out_phase].insert(sub);

        init(out_phase);
    }

    virtual ExplanationItem explain(ExplanationCtx ctx) const = 0;

    int phases() const { return phases_; }
    int minPhase() const { return min_out_phase_; }
    int maxPhase() const { return max_out_phase_; }
    const Subscribers& subscribers() const { return subs_; }

 protected:
    // the way for downstream operations to ask for output on phase `out_phase`
    // idempotent
    virtual void init(int out_phase) = 0;

    bool hasSubscriber(int phase, const Subscriber* sub) const {
        auto it = subs_.find(phase);
        return it != subs_.end() && it->second.contains(const_cast<Subscriber*>(sub));  // NOLINT
    }

    bool active(int phase) const {
        auto it = subs_.find(phase);
        return it != subs_.end() && !it->second.empty();
    }

    // returns active(phase)
    bool emit(int phase, const exec::Record* record) {
        verify(active(phase));

        auto&& subs = subs_[phase];
        auto it = subs.begin();

        while (it != subs.end()) {
            bool cont = (*it)->consume(phase, record);

            if (cont) {
                ++it;
            } else {
                it = subs.erase(it);
            }
        }

        return active(phase);
    }

    // total phases in this operation
    const int phases_ = 0;

    // the phase the result is available at
    const int min_out_phase_ = 0;

 protected:
    // the max out phase
    int max_out_phase_ = 0;

    // global phase -> subscribers
    Subscribers subs_;
};

using OperationPtr = std::shared_ptr<Operation>;

}  // namespace lsql::exec

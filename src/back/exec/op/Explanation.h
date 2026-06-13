#pragma once

#include "back/exec/op/Subscriber.h"

#include "util/StrBuilder.h"

namespace lsql::back::exec {

class Operation;

using ExplanationItem = util::StrBuilder;

class Explanation {
 public:
    Explanation() = default;

    void insert(ExplanationItem item, const Operation* op) {
        if (item.empty()) {
            return;
        }

        items.emplace(op, std::move(item));
    }

    util::StrBuilder render() const {
        util::StrBuilder b;
        for (auto&& [_, item] : items) {
            b.item(item);
        }
        return b;
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

}  // namespace lsql::back::exec

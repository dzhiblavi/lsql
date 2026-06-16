#pragma once

#include "back/exec/op/Explanation.h"

#include "util/StrBuilder.h"

#include <span>
#include <string>

namespace lsql::back::exec {

template <typename T>
std::string explain(int max_phase, std::span<T> operations) {
    util::StrBuilder b;

    for (int phase = 0; phase <= max_phase; ++phase) {
        back::exec::Explanation explanation;
        back::exec::ExplanationCtx ctx{
            .requester = nullptr,
            .phase = phase,
            .explanation = explanation,
        };

        util::StrBuilder ops;

        for (auto&& op : operations) {
            auto explain = op->explain(ctx);

            if (!explain.empty()) {
                ops.item(explain);
            }
        }

        b.item("phase {}", phase).child(explanation.render()).child(ops);
    }

    return b.render();
}

}  // namespace lsql::back::exec

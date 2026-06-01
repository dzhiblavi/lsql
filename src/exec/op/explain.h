#pragma once

#include "exec/op/Explanation.h"

#include "util/StrBuilder.h"

#include <span>
#include <string>

namespace lsql::exec {

template <typename T>
inline std::string explain(int max_phase, std::span<T> operations) {
    util::StrBuilder b;

    for (int phase = 0; phase <= max_phase; ++phase) {
        exec::Explanation explanation;
        exec::ExplanationCtx ctx{
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

        b.item("phase {}", phase).child(ops);
    }

    return b.render();
}

}  // namespace lsql::exec

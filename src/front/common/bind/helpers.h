#pragma once

#include "front/common/bind/Context.h"
#include "front/common/source/SourceSpan.h"

#include <vector>

namespace lsql::front::common::bind {

template <typename BoundProjector, typename AstProjector>
std::vector<BoundProjector> bindProjectors(
    std::vector<AstProjector> projectors, auto& binder, Context& ctx) {
    std::vector<BoundProjector> result;
    result.reserve(projectors.size());
    for (auto&& p : projectors) {
        binder(std::move(p), result, ctx);
    }
    return result;
}

template <typename T>
SourceSpan spanOf(const std::vector<T>& args) {
    if (args.empty()) {
        return SourceSpan{};
    }

    return merge(args.front().span, args.back().span);
}

}  // namespace lsql::front::common::bind

#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>
#include <reflex/stdmatcher.h>

namespace lsql::func {

struct LikeExecutor {
    explicit LikeExecutor(const std::string& regex) : regex(regex), pattern(regex) {}
    LikeExecutor(const LikeExecutor& r) : LikeExecutor(r.regex) {}

    Value execute(const Value& value) const {
        auto view = value.get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        return reflex::Matcher(&pattern, input).matches() != 0;
    }

    std::string regex;
    reflex::Pattern pattern;
};

static_assert(UnaryExecutor<LikeExecutor>);

inline LikeExecutor build(const Like& s) {
    return LikeExecutor(s.regex);
}

}  // namespace lsql::func

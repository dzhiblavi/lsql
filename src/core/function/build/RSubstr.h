#pragma once

#include "core/function/Executor.h"
#include "core/function/Function.h"

#include <reflex/matcher.h>
#include <reflex/pattern.h>
#include <reflex/stdmatcher.h>

namespace lsql::func {

struct RSubstrExecutor {
    explicit RSubstrExecutor(const std::string& regex) : regex(regex), pattern(regex) {}
    RSubstrExecutor(const RSubstrExecutor& e) : RSubstrExecutor(e.regex) {}

    Value execute(const Value& value) const {
        auto view = value.get<std::string_view>();
        auto input = reflex::Input(view.data(), view.size());
        reflex::Matcher matcher(&pattern, input);

        size_t group = matcher.find();
        if (group == 0) {
            return null;
        }

        return value.substr(matcher.first(), matcher.size());
    }

    std::string regex;
    reflex::Pattern pattern;
};

static_assert(UnaryExecutor<RSubstrExecutor>);

inline RSubstrExecutor build(const RSubstr& s) {
    return RSubstrExecutor(s.regex);
}

}  // namespace lsql::func

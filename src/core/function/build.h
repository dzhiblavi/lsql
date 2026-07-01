#pragma once

#include "core/function/Concepts.h"

#include "core/function/build/Add.h"
#include "core/function/build/And.h"
#include "core/function/build/BooleanNegate.h"
#include "core/function/build/Cast.h"
#include "core/function/build/Coalesce.h"
#include "core/function/build/Comparison.h"
#include "core/function/build/CountAll.h"
#include "core/function/build/CountNonNull.h"
#include "core/function/build/Divide.h"
#include "core/function/build/Equal.h"
#include "core/function/build/Like.h"
#include "core/function/build/Lower.h"
#include "core/function/build/Max.h"
#include "core/function/build/Min.h"
#include "core/function/build/NotEqual.h"
#include "core/function/build/Or.h"
#include "core/function/build/ParseTimestamp.h"
#include "core/function/build/Percentile.h"
#include "core/function/build/RSubstr.h"
#include "core/function/build/SplitPart.h"
#include "core/function/build/Substr.h"
#include "core/function/build/Subtract.h"
#include "core/function/build/Sum.h"

namespace lsql::func {

template <typename R, typename F>
R buildScalar(const Function& func, F&& f) {
    return util::match(
        func,
        util::Overloaded{
            [&](const Lower& s) -> R { return f(build(s)); },
            [&](const SplitPart& s) -> R { return f(build(s)); },
            [&](const Substr& s) -> R { return f(build(s)); },
            [&](const Coalesce& s) -> R { return f(build(s)); },
            [&](const RSubstr& s) -> R { return f(build(s)); },
            [&](const Like& s) -> R { return f(build(s)); },
            [&](const Cast& s) -> R { return f(build(s)); },
            [&](const ParseTimestamp& s) -> R { return f(build(s)); },
            [&](const BooleanNegate& s) -> R { return f(build(s)); },
            [&](const Equal& s) -> R { return f(build(s)); },
            [&](const NotEqual& s) -> R { return f(build(s)); },
            [&](const Less& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Greater& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const LessEqual& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const GreaterEqual& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const And& s) -> R { return f(build(s)); },
            [&](const Or& s) -> R { return f(build(s)); },
            [&](const Divide& s) -> R {
                return dispatch<R>(
                    [&]<Dividable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Add& s) -> R {
                return dispatch<R>(
                    [&]<Addable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Subtract& s) -> R {
                return dispatch<R>(
                    [&]<Subtractable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [](auto&&...) -> R { panic("not a supported scalar"); },
        });
}

template <typename R, typename F>
R buildAggregate(const Function& func, F&& f) {
    return util::match(
        func,
        util::Overloaded{
            [&](const CountNonNull& s) -> R { return f(build(s)); },
            [&](const CountAll& s) -> R { return f(build(s)); },
            [&](const Percentile& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.args_type);
            },
            [&](const Min& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Max& s) -> R {
                return dispatch<R>(
                    [&]<Comparable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [&](const Sum& s) -> R {
                return dispatch<R>(
                    [&]<Addable T>(std::type_identity<T>) -> R { return f(build<T>(s)); },
                    s.arg_type);
            },
            [](auto&&...) -> R { panic("not a supported aggregate"); },
        });
}

}  // namespace lsql::func

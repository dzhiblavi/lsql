#pragma once

#include <llog/log.h>

#define panic(...)                         \
    llog::critical("panic: " __VA_ARGS__); \
    std::terminate();                      \
    __builtin_unreachable()

#define verify(X, ...)                                                 \
    [&] {                                                              \
        auto&& res = X;                                                \
                                                                       \
        if (!static_cast<bool>(res)) [[unlikely]] {                    \
            panic("verify(" #X ") failed" __VA_OPT__(, ) __VA_ARGS__); \
        }                                                              \
                                                                       \
        return std::forward<decltype(X)>(res);                         \
    }()

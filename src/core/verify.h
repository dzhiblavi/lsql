#pragma once

#include <llog/log.h>

#define verify(X, ...)                                                          \
    [&] {                                                                       \
        auto&& res = X;                                                         \
                                                                                \
        if (!static_cast<bool>(res)) [[unlikely]] {                             \
            llog::critical("verify(" #X ") failed" __VA_OPT__(, ) __VA_ARGS__); \
            std::terminate();                                                   \
        }                                                                       \
                                                                                \
        return std::forward<decltype(X)>(res);                                  \
    }()

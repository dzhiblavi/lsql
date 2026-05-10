#pragma once

#include <llog/log.h>

#define verify(...)                                            \
    [&] {                                                      \
        auto&& res = __VA_ARGS__;                              \
                                                               \
        if (!static_cast<bool>(res)) [[unlikely]] {            \
            llog::critical("verify(" #__VA_ARGS__ ") failed"); \
            std::terminate();                                  \
        }                                                      \
                                                               \
        return std::forward<decltype(__VA_ARGS__)>(res);       \
    }()

#pragma once

#include "util/build_info.h"

#include "util/logging.h"  // IWYU pragma: keep (llog::panic)

#define panic(...)                                     \
    [&] [[noreturn]] {                                 \
        ::lsql::llog::critical("panic: " __VA_ARGS__); \
        std::terminate();                              \
        __builtin_unreachable();                       \
    }()

#define verify(X, ...)                                      \
    [&] {                                                   \
        auto&& verify__res = X;                             \
                                                            \
        if (!static_cast<bool>(verify__res)) [[unlikely]] { \
            panic("verify(" #X ") failed " __VA_ARGS__);    \
        }                                                   \
                                                            \
        return std::forward<decltype(X)>(verify__res);      \
    }()

#if defined(LSQL_BUILD_DEBUG)

#define verify_dbg verify

#else

#define verify_dbg(...)

#endif

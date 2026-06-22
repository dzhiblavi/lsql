#pragma once

#include <spdlog/logger.h>

#include <concepts>
#include <source_location>

namespace lsql::llog {

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

LoggerPtr global();

namespace detail {

template <typename T>
concept WithLogger = requires(const T& t) {
    { t.getLogger() } -> std::same_as<LoggerPtr>;
};

template <typename T>
concept AsLogger =
    std::same_as<T, LoggerPtr> || WithLogger<T> || WithLogger<std::remove_pointer_t<T>>;

LoggerPtr getLogger(const WithLogger auto& x) {
    return x.getLogger();
}

LoggerPtr getLogger(const WithLogger auto* x) {
    return x->getLogger();
}

inline LoggerPtr getLogger(LoggerPtr logger) {
    return logger;
}

}  // namespace detail

template <detail::AsLogger Logger, typename... Args>
struct log {  // NOLINT
    log(const Logger& logger,
        spdlog::level::level_enum level,
        fmt::format_string<const Args&...> fmt,
        const Args&... args,
        const std::source_location& loc = std::source_location::current()) {
        auto spdlog_loc = spdlog::source_loc(loc.file_name(), int(loc.line()), loc.function_name());
        detail::getLogger(logger)->log(spdlog_loc, level, fmt::format(fmt, args...));
    }

    log(const Logger& logger,
        spdlog::level::level_enum level,
        const Args&... args,
        const std::source_location& loc = std::source_location::current()) {
        auto spdlog_loc = spdlog::source_loc(loc.file_name(), int(loc.line()), loc.function_name());
        detail::getLogger(logger)->log(spdlog_loc, level, args...);
    }
};

template <detail::AsLogger Logger, typename... Args>
log(const Logger&, spdlog::level::level_enum, fmt::format_string<const Args&...>, const Args&...)
    -> log<Logger, Args...>;

template <detail::AsLogger Logger, typename Msg>
log(const Logger&, spdlog::level::level_enum, const Msg&) -> log<Logger, Msg>;

// NOLINTNEXTLINE
#define LSQL_DEFINE_LOG_FOR_LEVEL(L)                                                            \
    template <detail::AsLogger Logger, typename... Args>                                        \
    struct L {                                                                                  \
        L(const Logger& logger,                                                                 \
          fmt::format_string<const Args&...> fmt,                                               \
          const Args&... args,                                                                  \
          const std::source_location& loc = std::source_location::current()) {                  \
            auto spdlog_loc =                                                                   \
                spdlog::source_loc(loc.file_name(), loc.line(), loc.function_name());           \
            detail::getLogger(logger)->log(                                                     \
                spdlog_loc, spdlog::level::L, fmt::format(fmt, args...));                       \
        }                                                                                       \
        L(const Logger& logger,                                                                 \
          const Args&... args,                                                                  \
          const std::source_location& loc = std::source_location::current()) {                  \
            auto spdlog_loc =                                                                   \
                spdlog::source_loc(loc.file_name(), loc.line(), loc.function_name());           \
            detail::getLogger(logger)->log(spdlog_loc, spdlog::level::L, args...);              \
        }                                                                                       \
        L(fmt::format_string<const Args&...> fmt,                                               \
          const Args&... args,                                                                  \
          const std::source_location& loc = std::source_location::current()) {                  \
            auto spdlog_loc =                                                                   \
                spdlog::source_loc(loc.file_name(), loc.line(), loc.function_name());           \
            global()->log(spdlog_loc, spdlog::level::L, fmt::format(fmt, args...));             \
        }                                                                                       \
        L(const Args&... args,                                                                  \
          const std::source_location& loc = std::source_location::current()) {                  \
            auto spdlog_loc =                                                                   \
                spdlog::source_loc(loc.file_name(), loc.line(), loc.function_name());           \
            global()->log(spdlog_loc, spdlog::level::L, args...);                               \
        }                                                                                       \
    };                                                                                          \
                                                                                                \
    template <typename... Args>                                                                 \
    L(fmt::format_string<const Args&...>, const Args&...) -> L<LoggerPtr, Args...>;             \
    template <typename Msg>                                                                     \
    L(const Msg&) -> L<LoggerPtr, Msg>;                                                         \
    template <detail::AsLogger Logger, typename... Args>                                        \
    L(const Logger&, fmt::format_string<const Args&...>, const Args&...) -> L<Logger, Args...>; \
    template <detail::AsLogger Logger, typename Msg>                                            \
    L(const Logger&, const Msg&)->L<Logger, Msg>

LSQL_DEFINE_LOG_FOR_LEVEL(trace);     // NOLINT
LSQL_DEFINE_LOG_FOR_LEVEL(debug);     // NOLINT
LSQL_DEFINE_LOG_FOR_LEVEL(info);      // NOLINT
LSQL_DEFINE_LOG_FOR_LEVEL(warn);      // NOLINT
LSQL_DEFINE_LOG_FOR_LEVEL(err);       // NOLINT
LSQL_DEFINE_LOG_FOR_LEVEL(critical);  // NOLINT

#undef LSQL_DEFINE_LOG_FOR_LEVEL

}  // namespace lsql::llog

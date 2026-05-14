#pragma once

#include "util/instrument/Counters.h"

#include <format>
#include <string>

namespace lsql::exec::prof {

class Metric {
 public:
    virtual ~Metric() = default;

    virtual std::string format() const = 0;
    virtual void reset() = 0;
};

template <std::integral T>
class NamedCounter : public Metric {
 public:
    NamedCounter(std::string_view name, T init) : init(init), name(name), counter(init) {}

    void reset() override { counter.set(init); }
    std::string format() const override { return std::format("{}: {}", name, counter.value()); }

    int64_t init;
    std::string_view name;
    instr::Counter<T> counter;
};

class Message : public Metric {
 public:
    Message() = default;

    template <typename... Args>
    void set(std::format_string<const Args&...> fmt, const Args&... args) {
        message = std::format(fmt, args...);
    }

    void reset() override { message.clear(); }
    std::string format() const override { return message; }

    std::string message;
};

}  // namespace lsql::exec::prof

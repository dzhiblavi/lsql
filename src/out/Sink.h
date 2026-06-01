#pragma once

#include <string_view>

namespace lsql::out {

template <typename S>
concept Sink = requires(S& sink, std::string_view s) {
    { sink.push(s) } -> std::same_as<void>;
    { sink.done() } -> std::same_as<void>;
};

}  // namespace lsql::out

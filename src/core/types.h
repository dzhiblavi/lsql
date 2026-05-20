#pragma once

#include <cstdint>
#include <memory>

namespace lsql {

using timestamp_t = int64_t;

template <typename T>
using Box = std::unique_ptr<T>;

template <typename T>
using Arc = std::shared_ptr<T>;

}  // namespace lsql

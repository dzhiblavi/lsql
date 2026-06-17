#pragma once

#include "core/types.h"

#include <cstddef>
#include <span>
#include <string>

namespace lsql::back::storage {

class Stream {
 public:
    virtual ~Stream() = default;
    virtual size_t read(size_t max_count, std::span<char> dest) = 0;
};

class StreamSource {
 public:
    virtual ~StreamSource() = default;
    virtual Box<Stream> stream() const = 0;
    virtual std::string describe() const { return "stream"; }
};

}  // namespace lsql::back::storage

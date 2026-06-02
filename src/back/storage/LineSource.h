#pragma once

#include "back/storage/PagedFile.h"

#include <coro/generator.hpp>

#include <cstddef>
#include <string_view>

namespace lsql::back::storage {

struct Line {
 public:
    Line(std::shared_ptr<const char> pin, size_t size) : pin_(std::move(pin)), size_(size) {}

    std::string_view view() const { return {pin_.get(), size_}; }

 private:
    std::shared_ptr<const char> pin_;
    size_t size_;
};

class LineSource {
 public:
    virtual ~LineSource() = default;

    virtual coro::generator<Line> lines() const = 0;
    virtual std::string describe() const = 0;
};

// LineSource backed by paged file
class PagedLineSource : public LineSource {
 public:
    explicit PagedLineSource(std::shared_ptr<PagedFile> file);

    PagedLineSource(std::shared_ptr<PagedFile> file, size_t begin, size_t end)
        : file_{std::move(file)}
        , begin_{begin}
        , end_{end} {}

    coro::generator<Line> lines() const override;
    std::string describe() const override;

 private:
    std::shared_ptr<PagedFile> file_;
    size_t begin_ = 0;
    size_t end_ = 0;
};

}  // namespace lsql::back::storage

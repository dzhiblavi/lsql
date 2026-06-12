#pragma once

#include "back/storage/PagedFile.h"
#include "back/storage/Stream.h"
#include "util/verify.h"

#include <coro/generator.hpp>

#include <cstddef>
#include <string_view>

namespace lsql::back::storage {

struct Line {
 public:
    Line() = default;
    Line(std::shared_ptr<const char> pin, size_t size) : pin_(std::move(pin)), size_(size) {}

    std::string_view view() const { return {pin_.get(), size_}; }
    const Arc<const char>& pin() const { return pin_; }

 private:
    std::shared_ptr<const char> pin_ = nullptr;
    size_t size_ = 0;
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
    explicit PagedLineSource(Arc<PagedFile> file);

    PagedLineSource(Arc<PagedFile> file, size_t begin, size_t end)
        : file_{std::move(file)}
        , begin_{begin}
        , end_{end} {}

    coro::generator<Line> lines() const override;
    std::string describe() const override;

 private:
    Arc<PagedFile> file_;
    size_t begin_ = 0;
    size_t end_ = 0;
};

class StreamLineSource : public LineSource {
 public:
    explicit StreamLineSource(Arc<StreamSource> source) : source_(std::move(source)) {
        verify(source_ != nullptr);
    }

    coro::generator<Line> lines() const override;
    std::string describe() const override;

 private:
    Arc<StreamSource> source_;
};

}  // namespace lsql::back::storage

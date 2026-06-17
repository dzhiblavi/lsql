#include "back/storage/LineSource.h"

#include "util/PageSize.h"

#include "config/build_settings.h"

#include <llog/log.h>

#include <cassert>
#include <span>

namespace lsql::back::storage {

PagedLineSource::PagedLineSource(Arc<PagedFile> file)
    : file_(std::move(file))
    , end_(file_->size()) {
}

coro::generator<Line> PagedLineSource::lines() const {
    llog::info(
        "scanning lines of '{}' in range [{}, {}) ({} bytes)",
        file_->path().c_str(),
        begin_,
        end_,
        end_ - begin_);

    size_t page_index = begin_ / util::pageSize();
    size_t pos = begin_;  // global position
    auto page = file_->page(page_index);

    auto page_included = [this](size_t page) { return page * util::pageSize() < end_; };
    auto next_page = [&] {
        if (!page_included(page_index + 1)) {
            return false;
        }

        ++page_index;
        page = file_->page(page_index);
        return true;
    };

    while (pos < end_) {
        auto data = page->data().substr(pos % util::pageSize());
        auto newline = data.find('\n');

        if (newline == std::string::npos) {
            auto line = arc<std::string>();
            line->append(data);

            while (newline == std::string::npos) {
                if (!next_page()) {
                    // failed to find \n at all
                    co_return;
                }

                data = page->data();
                newline = data.find('\n');
                line->append(data.substr(0, newline));

                if (newline == std::string::npos) {
                    // try to add the next page
                    continue;
                }

                co_yield Line({line, line->data()}, line->size());

                if (newline + 1 < data.size()) {
                    pos = util::pageSize() * page_index + newline + 1;
                } else {
                    // \n has been the last character in the page
                    if (!next_page()) {
                        co_return;
                    }

                    pos = util::pageSize() * page_index;
                }

                break;
            }
        } else {
            pos += newline + 1;
            if (pos > end_) {
                co_return;
            }

            co_yield Line({page, data.data()}, newline);

            if (newline + 1 == data.size()) {
                // \n has been the last character in the page
                if (!next_page()) {
                    co_return;
                }
            }
        }
    }
}

std::string PagedLineSource::describe() const {
    return std::format(
        "'{}' in range [{}, {}) ({} bytes)", file_->path().c_str(), begin_, end_, end_ - begin_);
}

coro::generator<Line> StreamLineSource::lines() const {
    llog::info("scanning lines of stream");

    verify(source_ != nullptr);
    auto stream = source_->stream();
    Arc<std::string> partial;

    while (true) {
        auto chunk = arc<std::string>(config::Buffering::DecompressedChunkSize, '\0');
        auto read = stream->read(chunk->size(), std::span<char>(chunk->data(), chunk->size()));
        if (read == 0) {
            if (partial && !partial->empty()) {
                co_yield Line({partial, partial->data()}, partial->size());
            }
            co_return;
        }

        chunk->resize(read);
        std::string_view data(*chunk);
        size_t begin = 0;

        while (begin < data.size()) {
            const size_t newline = data.find('\n', begin);
            if (newline == std::string_view::npos) {
                if (!partial) {
                    partial = arc<std::string>();
                }
                partial->append(data.substr(begin));
                break;
            }

            if (partial) {
                partial->append(data.substr(begin, newline - begin));
                co_yield Line({partial, partial->data()}, partial->size());
                partial.reset();
            } else {
                co_yield Line({chunk, chunk->data() + begin}, newline - begin);  // NOLINT
            }

            begin = newline + 1;
        }
    }
}

std::string StreamLineSource::describe() const {
    return source_->describe();
}

}  // namespace lsql::back::storage

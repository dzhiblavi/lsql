#pragma once

#include "back/storage/Page.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace lsql::back::storage {

class File {
 public:
    virtual ~File() = default;

    virtual const std::filesystem::path& path() const = 0;
    virtual size_t size() const = 0;
    virtual size_t read(size_t offset, std::span<char> dest) const = 0;
};

class PagedFile : public File {
 public:
    virtual ~PagedFile() = default;

    virtual size_t pageCount() const = 0;
    virtual std::shared_ptr<Page> page(size_t index) const = 0;
};

class NativePagedFile : public PagedFile, public util::NonCopyable {
 public:
    ~NativePagedFile();
    NativePagedFile(int fd, std::filesystem::path path);
    NativePagedFile(NativePagedFile&& rhs) noexcept;
    NativePagedFile& operator=(NativePagedFile&& rhs) noexcept;

    size_t size() const override;
    size_t pageCount() const override;
    std::shared_ptr<Page> page(size_t index) const override;
    size_t read(size_t offset, std::span<char> dest) const override;
    const std::filesystem::path& path() const override;

    static std::shared_ptr<NativePagedFile> open(std::filesystem::path path);

 private:
    int fd_ = -1;
    size_t size_;
    std::filesystem::path path_;
};

}  // namespace lsql::back::storage

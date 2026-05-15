#include "data/PagedFile.h"
#include "core/verify.h"
#include "util/PageSize.h"

#include <fcntl.h>
#include <format>
#include <sys/mman.h>
#include <unistd.h>

namespace lsql::data {

NativePagedFile::NativePagedFile(int fd, std::filesystem::path path)
    : fd_(fd)
    , size_(lseek(fd_, 0, SEEK_END))
    , path_(std::move(path)) {
}

NativePagedFile::NativePagedFile(NativePagedFile&& rhs) noexcept
    : fd_(std::exchange(rhs.fd_, -1))
    , size_(rhs.size_)
    , path_(std::move(rhs.path_)) {
}

NativePagedFile& NativePagedFile::operator=(NativePagedFile&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    std::swap(fd_, rhs.fd_);
    std::swap(size_, rhs.size_);
    std::swap(path_, rhs.path_);
    return *this;
}

NativePagedFile::~NativePagedFile() {
    if (fd_ == -1) {
        return;
    }

    ::close(std::exchange(fd_, -1));
}

size_t NativePagedFile::size() const {
    return size_;
}

std::shared_ptr<NativePagedFile> NativePagedFile::open(std::filesystem::path path) {
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error(
            std::format(
                "failed to open file '{}': errno={}, error={}",
                path.c_str(),
                errno,
                strerror(errno)));
    }

    return std::make_shared<NativePagedFile>(fd, std::move(path));
}

size_t NativePagedFile::pageCount() const {
    return util::pageCount(size());
}

std::shared_ptr<Page> NativePagedFile::page(size_t index) const {
    verify(index * util::pageSize() < size());

    size_t offset = index * util::pageSize();
    size_t size = std::min(util::pageSize(), size_ - offset);

    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd_,
                        offset);                // NOLINT
    if (addr == reinterpret_cast<void*>(-1)) {  // NOLINT
        throw std::runtime_error(
            std::format(
                "failed to map a page {}: errno={}, error={}", index, errno, strerror(errno)));
    }

    std::ignore = ::madvise(addr, size, MADV_SEQUENTIAL);
    return std::make_shared<MappedPage>(addr, size);
}

size_t NativePagedFile::read(size_t offset, std::span<char> dest) const {
    verify(offset < size());

    if (lseek(fd_, offset, SEEK_SET) == -1) {  // NOLINT
        throw std::runtime_error(
            std::format("lseek failed: errno={}, error={}", errno, strerror(errno)));
    }

    size_t read = ::read(fd_, dest.data(), dest.size());
    if (read == static_cast<size_t>(-1)) {
        throw std::runtime_error(
            std::format("read failed: errno={}, error={}", errno, strerror(errno)));
    }

    return read;
}

const std::filesystem::path& NativePagedFile::path() const {
    return path_;
}

}  // namespace lsql::data

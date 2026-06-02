#include "back/storage/Page.h"
#include "util/verify.h"

#include <cassert>
#include <sys/mman.h>
#include <tuple>  // std::ignore
#include <unistd.h>

namespace lsql::back::storage {

MappedPage::MappedPage(void* addr, size_t size) : addr_(addr), size_(size) {
}

MappedPage::MappedPage(MappedPage&& rhs) noexcept
    : addr_(std::exchange(rhs.addr_, nullptr))
    , size_(std::exchange(rhs.size_, 0)) {
    verify(addr_ != nullptr);
    verify(size_ > 0);
}

MappedPage& MappedPage::operator=(MappedPage&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    std::swap(rhs.addr_, addr_);
    std::swap(rhs.size_, size_);
    return *this;
}

MappedPage::~MappedPage() {
    if (addr_ == nullptr) {
        return;
    }

    std::ignore = munmap(addr_, size_);
}

std::string_view MappedPage::data() const {
    verify_dbg(addr_ != nullptr);
    return {static_cast<const char*>(addr_), size_};
}

}  // namespace lsql::back::storage

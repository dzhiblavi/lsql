#pragma once

#include "util/NonCopyable.h"

#include <cstddef>
#include <string_view>

namespace lsql::back::storage {

class Page {
 public:
    virtual ~Page() = default;
    virtual std::string_view data() const = 0;
};

class MappedPage : public Page, util::NonCopyable {
 public:
    MappedPage(void* addr, size_t size);
    MappedPage(MappedPage&& rhs) noexcept;
    MappedPage& operator=(MappedPage&& rhs) noexcept;

    ~MappedPage();

    std::string_view data() const override;

 private:
    void* addr_ = nullptr;
    size_t size_ = 0;
};

}  // namespace lsql::back::storage

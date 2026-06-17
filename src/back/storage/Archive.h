#pragma once

#include "back/storage/PagedFile.h"
#include "back/storage/Stream.h"

#include "core/types.h"
#include "util/verify.h"

namespace lsql::back::storage {

class NativeArchive : public StreamSource {
 public:
    explicit NativeArchive(Arc<File> file) : file_(std::move(file)) { verify(file_ != nullptr); }
    Box<Stream> stream() const override;
    std::string describe() const override;

 private:
    Arc<File> file_;
};

}  // namespace lsql::back::storage

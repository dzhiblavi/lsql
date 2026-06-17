#pragma once

#include "back/storage/Stream.h"

namespace lsql::back::storage {

class CommandStreamSource : public StreamSource {
 public:
    explicit CommandStreamSource(std::string command) : command_(std::move(command)) {}

    Box<Stream> stream() const override;
    std::string describe() const override;

 private:
    std::string command_;
};

}  // namespace lsql::back::storage

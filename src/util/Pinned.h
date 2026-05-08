#pragma once

namespace lsql::util {

struct Pinned {
    Pinned() = default;
    Pinned(const Pinned&) = delete;
    Pinned& operator=(const Pinned&) = delete;
    Pinned(Pinned&&) = delete;
    Pinned& operator=(Pinned&&) = delete;
};

}  // namespace lsql::util

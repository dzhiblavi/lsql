#include "util/archive.h"

#include <array>

namespace lsql::util {

bool isProbablyArchive(std::string_view path) {
    static constexpr std::array CompressedSuffixes{
        ".gz",
        ".gzip",
    };

    for (auto&& suff : CompressedSuffixes) {
        if (path.ends_with(suff)) {
            return true;
        }
    }

    return false;
}

}  // namespace lsql::util

#include "util/logging.h"

#include <spdlog/spdlog.h>

namespace lsql::llog {

LoggerPtr global() {
    return spdlog::default_logger();
}

}  // namespace lsql::llog

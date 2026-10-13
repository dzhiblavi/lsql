#include "util/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace lsql::llog {

LoggerPtr global() {
    static auto logger = spdlog::stderr_color_mt("logsql");
    return logger;
}

}  // namespace lsql::llog

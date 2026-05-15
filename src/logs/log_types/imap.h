#include "core/time_formats.h"
#include "logs/log_types/anon_columns.h"
#include "logs/log_types/impl.h"

#include <cassert>
#include <string>

namespace lsql::logs {

template <>
struct LogTypeImpl<LogType::IMAP> {
    static constexpr TimeFormat time_format = TimeFormat::ORACLE;

    template <typename F>
    static void parseKeyValue(std::string_view line, F&& callback) {
        assert(line.size() >= 30);

        callback("lsql_line", line);
        auto timestamp = line.substr(1, 20);
        callback("timestamp", timestamp);
        size_t anon_index = 0;

        auto curr = line.substr(30);  // skip [timestamp]<space>

        while (!curr.empty()) {
            auto sep = curr.find(' ');
            auto token = curr.substr(0, sep);
            auto eq = token.find('=');

            if (eq == std::string::npos) {
                if (anon_index < AnonColumnNames.size()) {
                    callback(AnonColumnNames[anon_index++], token);
                }
            } else {
                auto key = token.substr(0, eq);
                auto value = token.substr(eq + 1);
                callback(key, value);
            }

            if (sep == std::string::npos) {
                return;
            }

            curr = curr.substr(sep + 1);
        }
    }

    static bool detectLogType(std::string_view line) { return line.starts_with("["); }
};

}  // namespace lsql::logs

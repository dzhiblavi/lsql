#include "back/logfmt/log_types/anon_columns.h"
#include "back/logfmt/log_types/impl.h"

#include "core/time_formats.h"
#include "util/verify.h"

#include <cassert>
#include <string>

namespace lsql::back::logfmt {

template <>
struct LogTypeImpl<LogType::IMAP> {
    static constexpr TimeFormat time_format = TimeFormat::ORACLE;

    template <typename F>
    static void parseKeyValue(std::string_view line, F&& callback) {
        verify_dbg(line.size() >= 21);

        auto timestamp = line.substr(1, 20);
        callback("timestamp", timestamp);
        size_t anon_index = 0;

        auto sep = line.find(' ');
        if (sep == std::string::npos) {
            return;
        }
        auto curr = line.substr(sep + 1);

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

}  // namespace lsql::back::logfmt

#include "back/logfmt/log_types/anon_columns.h"
#include "back/logfmt/log_types/impl.h"
#include "core/time_formats.h"

#include <string>

namespace lsql::back::logfmt {

template <>
struct LogTypeImpl<LogType::TSKV2> {
    static constexpr TimeFormat time_format = TimeFormat::SQL;

    template <typename F>
    static void parseKeyValue(std::string_view line, F&& callback) {
        auto curr = line;
        size_t anon_index = 0;

        while (!curr.empty()) {
            auto tab = curr.find('\t');
            auto token = curr.substr(0, tab);
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

            if (tab == std::string::npos) {
                return;
            }

            curr = curr.substr(tab + 1);
        }
    }

    static bool detectLogType(std::string_view line) { return line.contains("tskv"); }
};

}  // namespace lsql::back::logfmt

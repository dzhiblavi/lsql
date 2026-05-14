#include "logs/log_types/anon_columns.h"
#include "logs/log_types/impl.h"

#include <string>

namespace lsql::logs {

template <>
TimeFormat time_format<LogType::TSKV2> = TimeFormat::SQL;

template <>
void parseKeyValue<LogType::TSKV2>(
    std::string_view line, std::unordered_map<std::string_view, std::string_view>& out) {
    out["lsql_line"] = line;

    auto curr = line;
    size_t anon_index = 0;

    while (!curr.empty()) {
        auto tab = curr.find('\t');
        auto token = curr.substr(0, tab);
        auto eq = token.find('=');

        if (eq == std::string::npos) {
            if (anon_index < AnonColumnNames.size()) {
                out.emplace(AnonColumnNames[anon_index++], token);
            }
        } else {
            auto key = token.substr(0, eq);
            auto value = token.substr(eq + 1);
            out.emplace(key, value);
        }

        if (tab == std::string::npos) {
            return;
        }

        curr = curr.substr(tab + 1);
    }
}

template <>
bool detectLogType<LogType::TSKV2>(std::string_view line) {
    return line.contains("tskv");
}

}  // namespace lsql::logs

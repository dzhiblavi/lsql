#include "logs/log_types/anon_columns.h"
#include "logs/log_types/impl.h"

#include <cassert>
#include <string>

namespace lsql::logs {

template <>
TimeFormat time_format<LogType::IMAP> = TimeFormat::ORACLE;

template <>
void parseKeyValue<LogType::IMAP>(
    std::string_view line, std::unordered_map<std::string_view, std::string_view>& out) {
    assert(line.size() >= 30);

    out["lsql_line"] = line;
    auto timestamp = line.substr(1, 20);
    out["timestamp"] = timestamp;
    size_t anon_index = 0;

    auto curr = line.substr(30);  // skip [timestamp]<space>

    while (!curr.empty()) {
        auto sep = curr.find(' ');
        auto token = curr.substr(0, sep);
        auto eq = token.find('=');

        if (eq == std::string::npos) {
            out.emplace(AnonColumnNames[anon_index++], token);
        } else {
            auto key = token.substr(0, eq);
            auto value = token.substr(eq + 1);
            out.emplace(key, value);
        }

        if (sep == std::string::npos) {
            return;
        }

        curr = curr.substr(sep + 1);
    }
}

template <>
bool detectLogType<LogType::IMAP>(std::string_view line) {
    return line.starts_with("[");
}

}  // namespace lsql::logs

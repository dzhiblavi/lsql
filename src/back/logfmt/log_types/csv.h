#include "back/logfmt/log_types/anon_columns.h"
#include "back/logfmt/log_types/impl.h"
#include "core/time_formats.h"

#include <string>
#include <string_view>
#include <utility>

namespace lsql::back::logfmt {

template <>
struct LogTypeImpl<LogType::CSV> {
    static constexpr std::optional<TimeFormat> time_format = std::nullopt;

    template <typename F>
    static bool parseKeyValue(std::string_view line, F&& callback) {
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }

        size_t column_index = 0;
        while (true) {
            std::string_view field;
            size_t separator;
            bool escaped = false;

            if (line.starts_with('"')) {
                size_t position = 1;
                while (true) {
                    position = line.find('"', position);
                    if (position == std::string_view::npos) {
                        return false;
                    }
                    if (position + 1 < line.size() && line[position + 1] == '"') {
                        escaped = true;
                        position += 2;
                        continue;
                    }
                    break;
                }

                field = line.substr(1, position - 1);
                separator = position + 1;
                if (separator == line.size()) {
                    separator = std::string_view::npos;
                } else if (line[separator] != ',') {
                    return false;
                }
            } else {
                separator = line.find(',');
                field = line.substr(0, separator);
                if (field.contains('"')) {
                    return false;
                }
            }

            if (field.find_first_of("\r\n") != std::string_view::npos) {
                return false;
            }

            if (column_index < AnonColumnNames.size()) {
                if (escaped) {
                    std::string value;
                    value.reserve(field.size());
                    for (size_t position = 0; position < field.size(); ++position) {
                        value.push_back(field[position]);
                        if (field[position] == '"') {
                            ++position;
                        }
                    }
                    callback(AnonColumnNames[column_index], std::move(value));
                } else {
                    callback(AnonColumnNames[column_index], field);
                }
            }
            ++column_index;

            if (separator == std::string_view::npos) {
                return true;
            }
            line.remove_prefix(separator + 1);
        }
    }

    static bool detectLogType(std::string_view line) {
        if (!line.contains(',') || hasUnquotedTabs(line)) {
            return false;
        }

        auto cols = countColumns(line);
        return cols && *cols > 1;
    }

 private:
    static bool hasUnquotedTabs(std::string_view line) {
        bool quoted = false;

        for (char symbol : line) {
            if (symbol == '"') {
                quoted = !quoted;
            } else if (symbol == '\t' && !quoted) {
                return true;
            }
        }

        return false;
    }

    static std::optional<size_t> countColumns(std::string_view line) {
        size_t column_count = 0;

        if (!parseKeyValue(line, [&](std::string_view, auto&&) { ++column_count; })) {
            return std::nullopt;
        }

        return column_count;
    }
};

}  // namespace lsql::back::logfmt

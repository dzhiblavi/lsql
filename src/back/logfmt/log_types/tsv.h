#include "back/logfmt/log_types/anon_columns.h"
#include "back/logfmt/log_types/impl.h"
#include "core/time_formats.h"

#include <string_view>

namespace lsql::back::logfmt {

template <>
struct LogTypeImpl<LogType::TSV> {
    static constexpr std::optional<TimeFormat> time_format = std::nullopt;

    template <typename F>
    static bool parseKeyValue(std::string_view line, F&& callback) {
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }

        if (line.find_first_of("\r\n") != std::string_view::npos) {
            return false;
        }

        size_t column_index = 0;
        while (true) {
            const auto separator = line.find('\t');
            if (column_index < AnonColumnNames.size()) {
                callback(AnonColumnNames[column_index], line.substr(0, separator));
            }
            ++column_index;

            if (separator == std::string_view::npos) {
                return true;
            }
            line.remove_prefix(separator + 1);
        }
    }

    static bool detectLogType(std::string_view line) {
        return line.contains('\t') && parseKeyValue(line, [](auto&&...) {});
    }
};

}  // namespace lsql::back::logfmt

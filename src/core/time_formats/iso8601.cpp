#include "core/time_formats/impl.h"

#include "util/verify.h"

namespace lsql {

template <>
std::string_view timeFormatRegex<TimeFormat::ISO8601>() {
    return R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})";
}

template <>
timestamp_t timestampFromString<TimeFormat::ISO8601>(std::string_view s) {
    verify_dbg(s.size() >= 19);

    struct tm tm = {};
    tm.tm_year =
        ((s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0')) - 1900;
    tm.tm_mon = ((s[5] - '0') * 10 + (s[6] - '0')) - 1;
    tm.tm_mday = (s[8] - '0') * 10 + (s[9] - '0');
    tm.tm_hour = (s[11] - '0') * 10 + (s[12] - '0');
    tm.tm_min = (s[14] - '0') * 10 + (s[15] - '0');
    tm.tm_sec = (s[17] - '0') * 10 + (s[18] - '0');

    return mktime(&tm);
}

}  // namespace lsql

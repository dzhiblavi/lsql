#include "core/time_formats/impl.h"

namespace lsql {

template <>
std::string_view timeFormatRegex<TimeFormat::ORACLE>() {
    return R"(\d{4}-\w{3}-\d{2} \d{2}:\d{2}:\d{2})";
}

template <>
timestamp_t timestampFromString<TimeFormat::ORACLE>(std::string_view s) {
    assert(s.size() == 20);

    std::tm tm{};
    tm.tm_year = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0') - 1900;

    static constexpr int month_map[12][4] = {
        {'J', 'a', 'n', 1},
        {'F', 'e', 'b', 2},
        {'M', 'a', 'r', 3},
        {'A', 'p', 'r', 4},
        {'M', 'a', 'y', 5},
        {'J', 'u', 'n', 6},
        {'J', 'u', 'l', 7},
        {'A', 'u', 'g', 8},
        {'S', 'e', 'p', 9},
        {'O', 'c', 't', 10},
        {'N', 'o', 'v', 11},
        {'D', 'e', 'c', 12},
    };

    tm.tm_mon = -1;
    for (const auto& m : month_map) {
        if (s[5] == m[0] && s[6] == m[1] && s[7] == m[2]) {
            tm.tm_mon = m[3] - 1;
            break;
        }
    }
    assert(tm.tm_mon != -1);

    tm.tm_mday = (s[9] - '0') * 10 + (s[10] - '0');
    tm.tm_hour = (s[12] - '0') * 10 + (s[13] - '0');
    tm.tm_min = (s[15] - '0') * 10 + (s[16] - '0');
    tm.tm_sec = (s[18] - '0') * 10 + (s[19] - '0');

    return mktime(&tm);
}

}  // namespace lsql

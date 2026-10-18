#pragma once

namespace lsql::back::logfmt {

enum class LogType {
    TSKV,       // key=value\tkey=value\ttimestamp=<SQL timestamp>
    TS_LOGFMT,  // [<ORACLE timestamp>] field key=value key=value
    JSONL,      // {"key": "value", "key2": [...]}
    CSV,        // value,value,value
    TSV,        // value\tvalue\tvalue
};

}  // namespace lsql::back::logfmt

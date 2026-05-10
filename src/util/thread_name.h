#pragma once

#include "util/uniq_id.h"

#include <string>

namespace lsql::util {

namespace detail {

inline thread_local std::string thread_name_;

}  // namespace detail

inline void setThreadName(std::string name) {
    detail::thread_name_ = std::move(name);
}

inline std::string_view threadName() {
    return detail::thread_name_;
}

inline int threadId() {
    thread_local const int id = uniqId();
    return id;
}

}  // namespace lsql::util

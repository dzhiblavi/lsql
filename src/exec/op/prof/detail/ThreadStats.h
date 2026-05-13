#pragma once

#include "util/instrument/SequenceProfile.h"
#include <vector>

namespace lsql::exec::prof::detail {

struct ThreadSubscriberStats {
    uint32_t records_in = 0;
    instr::SequenceProfile<std::chrono::microseconds> consume_profile = {};

    void reset() {
        records_in = 0;
        consume_profile.reset();
    }

    bool empty() const { return records_in == 0; }
};

struct ThreadOperationStats {
    uint32_t records_out = 0;
    instr::SequenceProfile<std::chrono::microseconds> emit_profile = {};
    std::vector<std::string> custom_info;

    void reset() {
        records_out = 0;
        emit_profile.reset();
        custom_info.clear();
    }

    template <typename... Args>
    void custom(std::format_string<const Args&...> fmt, const Args&... args) {
        this->custom_info.push_back(std::format(fmt, args...));
    }

    bool empty() const { return records_out == 0 && custom_info.empty(); }
};

}  // namespace lsql::exec::prof::detail

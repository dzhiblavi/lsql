#pragma once

#include "util/instrument/SequenceProfile.h"

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

    void reset() {
        records_out = 0;
        emit_profile.reset();
    }

    bool empty() const { return records_out == 0; }
};

}  // namespace lsql::exec::prof::detail

#include "exec/op/Profiler.h"
#include "exec/op/Operation.h"

#include "util/instrument/SequenceProfile.h"

#include <format>
#include <sstream>
#include <unordered_map>

namespace lsql::exec {

Profiler::OperationHandle Profiler::registerOperation(
    const Operation* self, std::string_view name) {
    auto [it, _] = stats_.emplace(self, OperationStats{.name = name});
    return OperationHandle(&it->second);
}

std::string Profiler::report() const {
    std::stringstream ss;
    ss << std::format("ops_count={}\n", stats_.size());
    for (auto&& [op, stats] : stats_) {
        ss << std::format(
            "[id={} name={}] records_out={}\n", op->uniqId(), stats.name, stats.records_out);
        ss << std::format(" emit profile: {}\n", stats.emit_profile.format());

        for (auto&& [_, stats] : stats.inputs) {
            ss << std::format("  input records_in={}\n", stats.records_in);
            ss << std::format("  consume profile: {}\n", stats.consume_profile.format());
        }

        ss << '\n';
    }

    return ss.str();
}

Profiler& Profiler::profiler() {
    static Profiler profiler;
    return profiler;
}

void Profiler::reset() {
    for (auto&& [_, stats] : stats_) {
        stats.reset();
    }
}

}  // namespace lsql::exec

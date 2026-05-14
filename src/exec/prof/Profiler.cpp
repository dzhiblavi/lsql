#include "exec/prof/Profiler.h"
#include "exec/op/Operation.h"

#include <format>
#include <sstream>
#include <unordered_map>

namespace lsql::exec::prof {

namespace {

Profiler* current_ = nullptr;

}  // namespace

Profiler::Profiler(size_t num_threads) : num_threads_(num_threads) {
    verify(current_ == nullptr, "profiler already registered");
    current_ = this;
}

Profiler::~Profiler() {
    verify(current_ == this);
    current_ = nullptr;
}

OperationHandle Profiler::registerOperation(const Operation* self) {
    if (!profiler()) {
        return {};
    }

    return profiler()->registerOperationImpl(self);
}

std::string Profiler::report() {
    if (!profiler()) {
        return "";
    }

    return profiler()->reportImpl();
}

void Profiler::reset() {
    if (!profiler()) {
        return;
    }

    profiler()->resetImpl();
}

OperationHandle Profiler::registerOperationImpl(const Operation* self) {
    auto [it, _] = stats_.emplace(self, num_threads_);
    return OperationHandle(&it->second);
}

std::string Profiler::reportImpl() {
    std::stringstream ss;

    for (auto&& [op, stats] : stats_) {
        std::stringstream oss;

        for (size_t t = 0; t < num_threads_; ++t) {
            auto* tstats = stats.thread(t);
            if (tstats->empty()) {
                continue;
            }

            oss << std::format("    [thread={}]\n", t);
            oss << std::format("      records_out: {}\n", tstats->records_out);
            oss << std::format("      emit profile: {}\n", tstats->emit_profile.format());
        }

        if (!oss.str().empty() && !stats.metrics().empty()) {
            oss << std::format("  - metrics:\n");

            for (auto&& metric : stats.metrics()) {
                oss << std::format("      {}\n", metric->format());
            }
        }

        std::stringstream ass;

        for (auto&& [_, istats] : stats.inputs()) {
            std::stringstream iss;

            for (size_t t = 0; t < num_threads_; ++t) {
                auto* istat = istats.thread(t);
                if (istat->empty()) {
                    continue;
                }

                iss << std::format("    [thread={}]\n", t);
                iss << std::format("      records_in: {}\n", istat->records_in);
                iss << std::format("      consume profile: {}\n", istat->consume_profile.format());
            }

            if (!iss.str().empty()) {
                ass << "  - input\n" << iss.str();
            }
        }

        if (!oss.str().empty() || !ass.str().empty()) {
            ss << std::format("Operation {}\n", op->fullName());
        }
        if (!oss.str().empty()) {
            ss << "  - output\n" << oss.str();
        }
        if (!ass.str().empty()) {
            ss << ass.str();
        }
    }

    return ss.str();
}

void Profiler::resetImpl() {
    if (!profiler()) {
        return;
    }

    for (auto&& [_, stats] : profiler()->stats_) {
        stats.reset();
    }
}

Profiler* Profiler::profiler() {
    return current_;
}

}  // namespace lsql::exec::prof

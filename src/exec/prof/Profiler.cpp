#include "exec/prof/Profiler.h"
#include "exec/op/Operation.h"

#include "util/StrBuilder.h"

#include <format>
#include <sstream>
#include <unordered_map>

namespace lsql::exec::prof {

using util::StrBuilder;

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
    auto ops_b = StrBuilder();

    for (auto&& [op, stats] : stats_) {
        auto out_b = StrBuilder();

        bool any_stats = false;
        for (size_t t = 0; t < num_threads_; ++t) {
            auto* tstats = stats.thread(t);
            if (tstats->empty()) {
                continue;
            }

            any_stats = true;
            out_b.item(StrBuilder("[thread={}]", t)
                           .block(StrBuilder("records_out: {}", tstats->records_out))
                           .block(StrBuilder("emit_profile: {}", tstats->emit_profile.format())));
        }

        auto inp_b = StrBuilder();
        for (auto&& [_, istats] : stats.inputs()) {
            auto in_b = StrBuilder();

            for (size_t t = 0; t < num_threads_; ++t) {
                auto* istat = istats.thread(t);
                if (istat->empty()) {
                    continue;
                }

                any_stats = true;
                in_b.item(
                    StrBuilder("[thread={}]", t)
                        .child(StrBuilder("records_in: {}", istat->records_in))
                        .child(StrBuilder("consume_profile: {}", istat->consume_profile.format())));
            }

            if (!in_b.empty()) {
                inp_b.item(StrBuilder("input").block(in_b));
            }
        }

        auto metr_b = StrBuilder();
        if (any_stats && (!stats.metrics().empty() || !stats.transientMetrics().empty())) {
            for (auto&& metric : stats.metrics()) {
                metr_b.child(metric->format());
            }
            for (auto&& metric : stats.transientMetrics()) {
                metr_b.child(metric->format());
            }
        }

        if (out_b.empty() && inp_b.empty() && metr_b.empty()) {
            continue;
        }

        auto b = StrBuilder("operation {}", op->name());
        if (!inp_b.empty()) {
            b.child(StrBuilder("inputs").block(inp_b));
        }
        if (!metr_b.empty()) {
            b.child(StrBuilder("metrics").block(metr_b));
        }
        if (!out_b.empty()) {
            b.child(StrBuilder("outputs").block(out_b));
        }

        ops_b.item(b);
    }

    return ops_b.render();
}

void Profiler::resetImpl() {
    for (auto&& [_, stats] : profiler()->stats_) {
        stats.reset();
    }
}

Profiler* Profiler::profiler() {
    return current_;
}

}  // namespace lsql::exec::prof

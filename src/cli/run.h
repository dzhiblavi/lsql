#pragma once

#include "back/exec/op/Operation.h"
#include "back/exec/op/Subscriber.h"
#include "back/exec/op/explain.h"
#include "back/plan/plan.h"

#include "profiling/Profiler.h"
#include "profiling/global.h"
#include "profiling/presentation.h"

#include "output/factory.h"

#include "ir/Stringifier.h"
#include "optimize/optimize.h"
#include "util/ThreadPool.h"

#include <fstream>
#include <latch>

namespace lsql {

class ConsumerBridge : public back::exec::Subscriber {
 public:
    ConsumerBridge(back::exec::OperationPtr source, FieldSet fields, Box<output::Consumer> consumer)
        : source_(std::move(source))
        , consumer_(std::move(consumer)) {
        source_->subscribe(source_->minPhase(), this, fields);
    }

    back::exec::ExplanationItem explain(back::exec::ExplanationCtx ctx) const {
        auto source = source_->explain(ctx.withRequester(this));

        if (ctx.phase != source_->minPhase()) {
            verify(source.empty());
            return {};
        } else {
            verify(!source.empty());
            return back::exec::ExplanationItem().line("Print").child(source);
        }
    }

 private:
    prof::ScopeHandleBase scopeHandle() override { return prof::ScopeHandleBase(); }

    bool consume([[maybe_unused]] int phase, const back::exec::Record* record) override {
        verify_dbg(phase == source_->minPhase());

        if (record == nullptr) {
            consumer_->done();
            return false;
        }

        rec_.clear();
        for (auto id : record->ids()) {
            rec_.emplace(id, record->value(id));
        }

        consumer_->consume(rec_);
        return true;
    }

    output::Record rec_;
    back::exec::OperationPtr source_;
    Box<output::Consumer> consumer_;
};

struct StdoutSink {
    StdoutSink() = default;

    void push(std::string_view s) { buf_ << s << '\n'; }

    void done() {
        std::lock_guard lg(m_);
        std::cout << buf_.str() << '\n';
        buf_ = {};
    }

 private:
    inline static std::mutex m_;
    std::stringstream buf_;
};

struct Settings {
    bool print_optimization_report;
    bool print_ir_optimized;
    bool print_ir_unoptimized;
    bool dump_profile;
    bool dump_flamegraphs;
    bool dump_dot_graph;
    unsigned optimization_passes;
    bool explain;
    bool profiling_enabled;
    bool run;
    unsigned num_threads;
    output::Format out_format;
};

inline void run(int max_phase, const auto& sources, util::ThreadPool& tp, const Settings& s) {
    std::vector<prof::Profiler::Snapshot> snapshots;

    for (int phase = 0; phase <= max_phase; ++phase) {
        llog::info("executing phase {}", phase);
        std::latch latch(sources.size());

        for (auto source : sources) {
            tp.enqueue([source, phase, &latch] {
                try {
                    if (phase <= source->maxPhase()) {
                        source->push(phase);
                    }
                } catch (const std::exception& e) {
                    panic("unhandled exception: {}", e.what());
                }

                latch.count_down();
            });
        }

        latch.wait();
        llog::info("phase {} completed", phase);

        if (auto* prof = prof::globalProfiler()) {
            snapshots.push_back(prof->snapshot());
            prof->reset();
        }
    }

    if (s.dump_profile) {
        llog::info("dumping profile");

        for (size_t i = 0; i < snapshots.size(); ++i) {
            std::cerr << std::format(
                "profile [phase={}]\n{}", i, prof::formatProfile(snapshots[i]));
        }
    }

    if (s.dump_flamegraphs) {
        llog::info("dumping flamegraph to prof.<N>.folded");

        for (size_t i = 0; i < snapshots.size(); ++i) {
            std::ofstream ofs(std::format("prof.{}.folded", i));
            ofs << prof::formatFoldedStacks(snapshots[i]);
        }
    }

    if (s.dump_dot_graph) {
        llog::info("dumping dot visualization to prof.dot");

        std::ofstream ofs("prof.dot");
        ofs << prof::formatDot(snapshots);
    }
}

inline back::plan::Plan plan(ir::Program ir, const Settings& s) {
    if (s.print_ir_unoptimized) {
        std::cout << "Unoptimized IR dump:" << std::endl;
        std::cout << ir::Stringifier().print(ir).render() << std::endl;
    }

    opt::Context opt_ctx;
    for (unsigned i = 0; i < s.optimization_passes; ++i) {
        ir = opt::optimize(std::move(ir), opt_ctx);

        if (!opt_ctx.changes()) {
            llog::info("stopped at optimization pass {} due to no changes", i);
            break;
        }
    }

    if (s.print_optimization_report) {
        std::cout << opt_ctx.report() << std::endl;
    }

    if (s.print_ir_optimized) {
        std::cout << "Optimized IR dump:" << std::endl;
        std::cout << ir::Stringifier().print(ir).render() << std::endl;
    }

    return back::plan::plan(std::move(ir));
}

inline void run(ir::Program ir, Settings s) {
    s.profiling_enabled |= (s.dump_flamegraphs || s.dump_dot_graph || s.dump_profile);

    std::optional<prof::Profiler> profiler;
    if (s.profiling_enabled) {
        llog::info("enabling profiling [threads={}]", s.num_threads);
        profiler.emplace();
        prof::setGlobalProfiler(&profiler.value());
    } else {
        llog::info("profiling disabled");
    }

    llog::info("planning");
    auto [sources, top_operations, binding] = plan(std::move(ir), s);

    std::vector<Arc<ConsumerBridge>> ops;
    for (auto&& [op, fields] : top_operations) {
        ops.push_back(
            arc<ConsumerBridge>(
                op, fields, output::makeConsumer<StdoutSink>(s.out_format, binding)));
    }

    llog::info("determining phase count");
    int max_phase = 0;
    for (auto&& source : sources) {
        max_phase = std::max(max_phase, source->maxPhase());
    }

    if (s.explain) {
        std::cout << back::exec::explain(max_phase, std::span<Arc<ConsumerBridge>>(ops));
    }

    if (s.run) {
        util::ThreadPool pool(s.num_threads);
        run(max_phase, sources, pool, s);
        pool.stop();
        pool.join();
    }
}

}  // namespace lsql

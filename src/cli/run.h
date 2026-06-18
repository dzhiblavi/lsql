#pragma once

#include "back/exec/phys/build.h"
#include "back/exec/plan/Stringifier.h"
#include "back/exec/plan/plan.h"

#include "profiling/Profiler.h"
#include "profiling/global.h"
#include "profiling/presentation.h"

#include "output/factory.h"

#include "ir/Stringifier.h"
#include "optimize/optimize.h"

#include "util/OrderedSink.h"
#include "util/ThreadPool.h"

#include <fstream>
#include <latch>
#include <vector>

namespace lsql {

class ConsumerBridge : public back::exec::phys::Subscriber {
 public:
    ConsumerBridge(Schema schema, Box<output::Consumer> consumer) : consumer_(std::move(consumer)) {
        for (auto id : schema.fieldIds()) {
            rec_.emplace_back(id, null);
        }
    }

 private:
    bool consume(const back::exec::Record* record) override {
        if (record == nullptr) {
            consumer_->done();
            return false;
        }

        for (uint32_t slot = 0; slot < rec_.size(); ++slot) {
            rec_[slot].second = record->value(SlotId(slot));
        }

        consumer_->consume(rec_);
        return true;
    }

    prof::ScopeHandleBase scopeHandle() const override { return {}; }

    output::Record rec_;
    Box<output::Consumer> consumer_;
};

struct Settings {
    bool print_optimization_report;
    bool print_ir_optimized;
    bool print_ir_unoptimized;
    bool dump_profile;
    bool dump_flamegraphs;
    bool dump_dot_graph;
    bool is_diagnostic;
    unsigned optimization_passes;
    bool explain;
    unsigned num_threads;
    bool keep_output_order;
    output::Format out_format;
    std::optional<TimeRange> default_time_range;
};

inline void run(back::exec::phys::Program& program, util::ThreadPool& tp, const Settings& s) {
    verify(!program.phases.empty());
    int max_phase = program.phases.rbegin()->first;
    std::vector<prof::Profiler::Snapshot> snapshots;

    for (int phase = 0; phase <= max_phase; ++phase) {
        auto it = program.phases.find(phase);
        if (it == program.phases.end()) {
            continue;
        }
        auto&& p = it->second;

        llog::info("executing phase {} ({} sources)", phase, p.sources.size());
        std::latch latch(p.sources.size());  // NOLINT

        for (auto source : p.sources) {
            tp.enqueue([source, &latch] {
                try {
                    source->push();
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
                "profile [phase={}]\n{}\n", i, prof::formatProfile(snapshots[i]));
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

inline ir::Program optimize(ir::Program ir, const Settings& s) {
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

    return ir;
}

inline void run(ir::Program ir, Settings s) {
    auto profiling_enabled = s.dump_flamegraphs || s.dump_dot_graph || s.dump_profile;

    std::optional<prof::Profiler> profiler;
    if (profiling_enabled) {
        llog::info("enabling profiling [threads={}]", s.num_threads);
        profiler.emplace();
        prof::setGlobalProfiler(&profiler.value());
    } else {
        llog::info("profiling disabled");
    }

    llog::info("optimizing");
    ir = optimize(std::move(ir), s);

    llog::info("planning");
    auto plan = back::exec::plan::plan(std::move(ir), {.default_time_range = s.default_time_range});
    if (s.explain) {
        std::cout << back::exec::plan::Stringifier().print(plan) << std::endl;
    }

    if (s.is_diagnostic) {
        prof::setGlobalProfiler(nullptr);
        return;
    }

    llog::info("building physical operations");
    auto phys = back::exec::phys::build(plan);

    std::mutex print_lock;
    std::vector<Box<util::OrderedSink>> sinks;
    std::vector<Box<ConsumerBridge>> ops;
    for (auto&& [_, phase] : phys.phases) {
        for (auto&& [schema, operation] : phase.outputs) {
            auto sink = box<util::OrderedSink>(
                &std::cout, &print_lock, s.keep_output_order && !sinks.empty());

            if (!sinks.empty()) {
                sinks.back()->setNext(sink.get());
            }

            auto consumer = box<ConsumerBridge>(
                schema,
                output::makeConsumer<util::OrderedSink>(
                    sink.get(), s.out_format, plan.field_binding));

            operation->output(consumer.get());
            ops.push_back(std::move(consumer));
            sinks.push_back(std::move(sink));
        }
    }

    util::ThreadPool pool(s.num_threads);
    run(phys, pool, s);
    pool.stop();
    pool.join();
    prof::setGlobalProfiler(nullptr);
}

}  // namespace lsql

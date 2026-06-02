#include "back/exec/op/Operation.h"
#include "back/exec/op/explain.h"
#include "prof/global.h"
#include "prof/presentation.h"
#include "util/ThreadPool.h"

#include "out/CSVHeaderFormatter.h"
#include "out/Consumer.h"
#include "out/Formats.h"
#include "out/JSONFormatter.h"
#include "out/TSKVFormatter.h"

#include "front/sql/ast/Stringifier.h"
#include "front/sql/bound/Stringifier.h"
#include "ir/Stringifier.h"

#include "back/exec/plan/plan.h"
#include "opt/optimize.h"

#include "front/sql/bind/bind.h"
#include "front/sql/lower/lower.h"
#include "front/sql/parser/parse.h"

#include "ir/Relations.h"  // IWYU pragma: keep
#include "ir/Scalars.h"    // IWYU pragma: keep
#include "ir/Statement.h"  // IWYU pragma: keep

#include "util/build_info.h"
#include "util/require.h"

#include <llog/load.h>
#include <llog/log.h>

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

#include <fstream>
#include <latch>

namespace lsql {

class ConsumerBridge : public back::exec::Subscriber {
 public:
    ConsumerBridge(back::exec::OperationPtr source, FieldSet fields, Box<out::Consumer> consumer)
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
    prof::ScopeMetricsBase* profHandle() override { return nullptr; }

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

    out::Record rec_;
    back::exec::OperationPtr source_;
    Box<out::Consumer> consumer_;
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

TCLAP::UnlabeledValueArg<std::string> sql_file_arg{
    "path",
    "path to the query file",
    false,
    "",
    "filesystem path",
};

TCLAP::ValueArg<std::string> format_arg{
    "f",
    "format",
    "output format",
    false,
    "TSKV",
    "see lsql::out::Format enum",
};

TCLAP::ValueArg<std::string> log_level_arg{
    "l",
    "log-level",
    "log level",
    false,
    "Warn",
    "Trace/Debug/Info/Warn/Err/Critical/Off",
};

TCLAP::ValueArg<unsigned> threads_arg{
    "j",
    "threads",
    "max number of threads",
    false,
    1,
    "unsigned",
};

TCLAP::ValueArg<unsigned> optimize_passes_arg{
    "",
    "optimize-passes",
    "number of optimization passes",
    false,
    5,
    "unsigned",
};

bool run_query = true;

TCLAP::SwitchArg force_run_arg{
    "",
    "run",
    "force run query (useful with other flags)",
};

TCLAP::SwitchArg explain_arg{
    "e",
    "explain",
    "show execution plan",
};

TCLAP::SwitchArg print_ast_arg{
    "",
    "print-ast",
    "show AST",
};

TCLAP::SwitchArg print_bound_arg{
    "",
    "print-bound",
    "show bound AST",
};

TCLAP::SwitchArg print_ir_unoptimized_arg{
    "",
    "print-ir-unoptimized",
    "show IR",
};

TCLAP::SwitchArg print_optimization_report_arg{
    "",
    "print-optimize-report",
    "print optimization report",
};

TCLAP::SwitchArg print_ir_optimized_arg{
    "",
    "print-ir-optimized",
    "show IR",
};

bool any_profile_enabled = false;

TCLAP::SwitchArg profile_arg{
    "p",
    "profile",
    "enable profiling (printed to stderr)",
};

TCLAP::SwitchArg flamegraph_arg{
    "",
    "flamegraph",
    "build phase flamegraphs (dumped to prof.N.folded)",
};

TCLAP::SwitchArg dot_graph_arg{
    "",
    "dot-graph",
    "build .dot graph (dumped to prof.dot)",
};

bool parseArgs(std::span<const char*> argv) {
    TCLAP::CmdLine cmd{"tsql", ' ', formatBuildInfo()};
    cmd.add(&sql_file_arg);
    cmd.add(&format_arg);
    cmd.add(&log_level_arg);
    cmd.add(&force_run_arg);
    cmd.add(&explain_arg);
    cmd.add(&print_ast_arg);
    cmd.add(&print_bound_arg);
    cmd.add(&print_ir_unoptimized_arg);
    cmd.add(&print_optimization_report_arg);
    cmd.add(&print_ir_optimized_arg);
    cmd.add(&threads_arg);
    cmd.add(&optimize_passes_arg);
    cmd.add(&profile_arg);
    cmd.add(&flamegraph_arg);
    cmd.add(&dot_graph_arg);
    cmd.setExceptionHandling(false);

    try {
        cmd.parse(static_cast<int>(argv.size()), argv.data());
    } catch (const TCLAP::ArgException& e) {
        throw std::runtime_error(std::format("error for argument '{}': {}", e.argId(), e.error()));
    } catch (const TCLAP::ExitException& e) {
        return false;
    }

    if ((explain_arg || print_ast_arg || print_bound_arg || print_ir_unoptimized_arg ||
         print_optimization_report_arg || print_ir_optimized_arg) &&
        !force_run_arg) {
        run_query = false;
    }

    any_profile_enabled = profile_arg || flamegraph_arg || dot_graph_arg;
    return true;
}

back::exec::Plan makePlan(std::string maybe_path) {
    std::ifstream ifs;
    std::istream* is = [&] -> std::istream* {
        if (maybe_path.empty()) {
            return &std::cin;
        } else {
            ifs.open(maybe_path.c_str());
            require(ifs.is_open(), "cannot open query file '{}'", maybe_path);
            return &ifs;
        }
    }();

    auto ast = front::sql::parse::parse(*is);
    if (print_ast_arg) {
        std::cout << "AST dump:" << std::endl;
        std::cout << front::sql::ast::Stringifier().print(ast) << std::endl;
    }

    auto bound_ast = front::sql::bind::bind(std::move(ast));
    if (print_bound_arg) {
        std::cout << "Bound AST dump:" << std::endl;
        std::cout << front::sql::bound::Stringifier().print(bound_ast) << std::endl;
    }

    auto ir = front::sql::lower::lowerToIR(std::move(bound_ast));
    if (print_ir_unoptimized_arg) {
        std::cout << "Unoptimized IR dump:" << std::endl;
        std::cout << ir::Stringifier().print(ir).render() << std::endl;
    }

    opt::Context opt_ctx;
    for (unsigned i = 0; i < optimize_passes_arg.getValue(); ++i) {
        ir = opt::optimize(std::move(ir), opt_ctx);

        if (!opt_ctx.changes()) {
            llog::info("stopped at optimization pass {} due to no changes", i);
            break;
        }
    }
    if (print_optimization_report_arg) {
        std::cout << opt_ctx.report() << std::endl;
    }

    if (print_ir_optimized_arg) {
        std::cout << "Optimized IR dump:" << std::endl;
        std::cout << ir::Stringifier().print(ir).render() << std::endl;
    }

    return back::exec::plan(std::move(ir));
}

void run(int max_phase, const auto& sources, util::ThreadPool& tp) {
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

    if (profile_arg) {
        llog::info("dumping profile");

        for (size_t i = 0; i < snapshots.size(); ++i) {
            std::cerr << std::format(
                "profile [phase={}]\n{}", i, prof::formatProfile(snapshots[i]));
        }
    }

    if (flamegraph_arg) {
        llog::info("dumping flamegraph to prof.<N>.folded");

        for (size_t i = 0; i < snapshots.size(); ++i) {
            std::ofstream ofs(std::format("prof.{}.folded", i));
            ofs << prof::formatFoldedStacks(snapshots[i]);
        }
    }

    if (dot_graph_arg) {
        llog::info("dumping dot visualization to prof.dot");

        std::ofstream ofs("prof.dot");
        ofs << prof::formatDot(snapshots);
    }
}

Box<out::Consumer> makeConsumer(out::Format format, ConstFieldBindingPtr binding) {
    switch (format) {
        case out::Format::JSON:
            return box<out::JSONFormatter<StdoutSink>>(StdoutSink{}, binding);
        case out::Format::TSKV:
            return box<out::TSKVFormatter<StdoutSink>>(StdoutSink{}, binding);
        case out::Format::CSVHeader:
            return box<out::CSVHeaderFormatter<StdoutSink>>(StdoutSink{}, binding);
    }
}

void main(std::span<const char*> argv) {
    if (!parseArgs(argv)) {
        return;
    }

    auto log_level = magic_enum::enum_cast<llog::Level>(log_level_arg.getValue());
    require(log_level.has_value(), "invalid value for log-level: {}", log_level_arg.getValue());
    llog::global()->set_level(static_cast<spdlog::level::level_enum>(*log_level));

    auto format = magic_enum::enum_cast<out::Format>(format_arg.getValue());
    require(format.has_value(), "invalid value for format: {}", format_arg.getValue());

    std::optional<prof::Profiler> profiler;
    if (any_profile_enabled) {
        llog::info("enabling profiling [threads={}]", threads_arg.getValue());
        profiler.emplace();
        prof::setGlobalProfiler(&profiler.value());
    } else {
        llog::info("profiling disabled");
    }

    llog::info("parsing the query and building operations");
    auto [sources, top_operations, binding] = makePlan(sql_file_arg.getValue());

    llog::info("collecting print operations");
    std::vector<Arc<ConsumerBridge>> ops;
    for (auto&& [op, fields] : top_operations) {
        ops.push_back(arc<ConsumerBridge>(op, fields, makeConsumer(*format, binding)));
    }

    llog::info("determining phase count");
    int max_phase = 0;
    for (auto&& source : sources) {
        max_phase = std::max(max_phase, source->maxPhase());
    }

    if (explain_arg) {
        std::cout << back::exec::explain(max_phase, std::span<Arc<ConsumerBridge>>(ops));
    }

    if (run_query) {
        util::ThreadPool pool(threads_arg.getValue());
        run(max_phase, sources, pool);
        pool.stop();
        pool.join();
    }
}

}  // namespace lsql

int main(int argc, const char** argv) {
    try {
        lsql::main(std::span<const char*>(argv, argc));
        return 0;
    } catch (const std::exception& e) {
        llog::critical("error: {}", e.what());
        return 1;
    } catch (...) {
        llog::critical("unknown error");
        return 2;
    }
}

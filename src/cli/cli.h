#include "cli/run.h"
#include "out/Formats.h"

#include "ir/Relations.h"  // IWYU pragma: keep
#include "ir/Scalars.h"    // IWYU pragma: keep
#include "ir/Statement.h"  // IWYU pragma: keep

#include "util/build_info.h"
#include "util/require.h"

#include <llog/load.h>
#include <llog/log.h>

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

namespace lsql {

TCLAP::UnlabeledValueArg<std::string> query_file_arg{
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
    cmd.add(&query_file_arg);
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

// This can be overriden by different CLIs
ir::Program parseQuery(std::string maybe_path);

void cliMain(std::span<const char*> argv) {
    if (!parseArgs(argv)) {
        return;
    }

    auto log_level = magic_enum::enum_cast<llog::Level>(log_level_arg.getValue());
    require(log_level.has_value(), "invalid value for log-level: {}", log_level_arg.getValue());
    llog::global()->set_level(static_cast<spdlog::level::level_enum>(*log_level));

    auto format = magic_enum::enum_cast<out::Format>(format_arg.getValue());
    require(format.has_value(), "invalid value for format: {}", format_arg.getValue());

    auto settings = Settings{
        .print_optimization_report = print_optimization_report_arg,
        .print_ir_optimized = print_ir_optimized_arg,
        .print_ir_unoptimized = print_ir_unoptimized_arg,
        .profiling_enabled = any_profile_enabled,
        .dump_profile = profile_arg,
        .dump_flamegraphs = flamegraph_arg,
        .dump_dot_graph = dot_graph_arg,
        .explain = explain_arg,
        .run = run_query,
        .optimization_passes = optimize_passes_arg,
        .num_threads = threads_arg,
        .out_format = *format,
    };

    run(parseQuery(query_file_arg), settings);
}

}  // namespace lsql

int main(int argc, const char** argv) {
    try {
        lsql::cliMain(std::span<const char*>(argv, argc));
        return 0;
    } catch (const std::exception& e) {
        llog::critical("error: {}", e.what());
        return 1;
    } catch (...) {
        llog::critical("unknown error");
        return 2;
    }
}

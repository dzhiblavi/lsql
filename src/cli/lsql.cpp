#include "exec/op/Operation.h"
#include "exec/prof/Profiler.h"
#include "util/ThreadPool.h"

#include "iface/sql/ast/Stringifier.h"
#include "iface/sql/bind/Stringifier.h"
#include "ir/Stringifier.h"

#include "exec/plan/plan.h"

#include "iface/sql/bind/bind.h"
#include "iface/sql/lower/lower.h"
#include "iface/sql/parser/parse.h"

#include "ir/Expressions.h"  // IWYU pragma: keep
#include "ir/Relations.h"    // IWYU pragma: keep
#include "ir/Statement.h"    // IWYU pragma: keep

#include "iface/sql/bind/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bind/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bind/Statement.h"    // IWYU pragma: keep

#include "core/require.h"

#include <llog/load.h>
#include <llog/log.h>

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

#include <fstream>
#include <latch>

namespace lsql {

enum class Format {
    TSKV,
    JSON,
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
    "see Format enum in " __FILE_NAME__,
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

TCLAP::SwitchArg debug_ast_arg{
    "",
    "debug-ast",
    "show AST",
};

TCLAP::SwitchArg debug_bind_arg{
    "",
    "debug-bind",
    "show bound AST",
};

TCLAP::SwitchArg debug_ir_arg{
    "",
    "debug-ir",
    "show IR",
};

TCLAP::SwitchArg profile_arg{
    "p",
    "profile",
    "enable profiling (printed to stderr)",
};

void println(std::string_view s) {
    static std::mutex m;
    std::lock_guard lg(m);
    std::cout << s << '\n';
}

bool parseArgs(std::span<const char*> argv) {
    TCLAP::CmdLine cmd{"tsql", ' ', "0.0.1"};
    cmd.add(&sql_file_arg);
    cmd.add(&format_arg);
    cmd.add(&log_level_arg);
    cmd.add(&force_run_arg);
    cmd.add(&explain_arg);
    cmd.add(&debug_ast_arg);
    cmd.add(&debug_bind_arg);
    cmd.add(&debug_ir_arg);
    cmd.add(&threads_arg);
    cmd.add(&profile_arg);
    cmd.setExceptionHandling(false);

    try {
        cmd.parse(static_cast<int>(argv.size()), argv.data());
    } catch (const TCLAP::ArgException& e) {
        throw std::runtime_error(std::format("error for argument '{}': {}", e.argId(), e.error()));
    } catch (const TCLAP::ExitException& e) {
        return false;
    }

    if ((explain_arg || debug_ast_arg || debug_bind_arg || debug_ir_arg) && !force_run_arg) {
        run_query = false;
    }

    return true;
}

exec::Plan makePlan(std::string maybe_path) {
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

    auto ast = iface::sql::parse::parse(*is);
    if (debug_ast_arg) {
        std::cout << "AST dump:" << std::endl;
        std::cout << iface::sql::ast::Stringifier().print(ast) << std::endl;
    }
    auto bound_ast = iface::sql::bind::bind(std::move(ast));
    if (debug_bind_arg) {
        std::cout << "Bound AST dump:" << std::endl;
        std::cout << iface::sql::bind::Stringifier().print(bound_ast) << std::endl;
    }
    auto ir = iface::sql::lower::lowerToIR(std::move(bound_ast));
    if (debug_ir_arg) {
        std::cout << "IR dump:" << std::endl;
        std::cout << ir::Stringifier().print(ir) << std::endl;
    }
    return exec::plan(std::move(ir));
}

std::string escapeForJSON(const std::string& input) {
    std::ostringstream oss;

    for (char c : input) {
        switch (c) {
            case '"':
                oss << "\\\"";
                break;
            case '\\':
                oss << "\\\\";
                break;
            case '/':
                oss << "\\/";
                break;
            case '\b':
                oss << "\\b";
                break;
            case '\f':
                oss << "\\f";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                // Control characters (0x00-0x1F) should be escaped as \uXXXX
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u00" << std::hex << std::uppercase
                        << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
                break;
        }
    }

    return oss.str();
}

std::string toJSONStr(const Value& v) {
    return visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](const std::string& x) -> std::string {
                return std::format("\"{}\"", escapeForJSON(x));
            },
        },
        v);
}

class Print : public exec::Subscriber {
 public:
    Print(exec::OperationPtr source, FieldSet fields, Format format, ConstFieldBindingPtr binding)
        : source_(std::move(source))
        , format_(format)
        , binding_(std::move(binding)) {
        source_->subscribe(source_->minPhase(), this, fields);
    }

    exec::ExplanationItem explain(exec::ExplanationCtx ctx) const {
        auto source = source_->explain(ctx.withRequester(this));

        if (ctx.phase != source_->minPhase()) {
            verify(source.empty());
            return {};
        } else {
            verify(!source.empty());
            return exec::ExplanationItem().line("Print").child(source);
        }
    }

 private:
    void done() const {
        println(ss_.str());
        ss_.str() = "";
    }

    bool consume(int phase, const exec::Record* record) override {
        verify(phase == source_->minPhase());

        if (record == nullptr) {
            done();
            return false;
        }

        switch (format_) {
            case Format::TSKV:
                printRecordTSKV(*record);
                break;

            case Format::JSON:
                printRecordJSON(*record);
                break;
        }

        return true;
    }

    void printRecordTSKV(const exec::Record& record) {
        for (auto id : record.ids()) {
            ss_ << std::format("{}={}", binding_->name(id), to_string(record.value(id))) << '\t';
        }
        ss_ << '\n';
    }

    void printRecordJSON(const exec::Record& record) {
        if (record.ids().empty()) {
            ss_ << "{}\n";
            return;
        }

        ss_ << '{';
        for (auto id : record.ids()) {
            ss_ << std::format("\"{}\":{}", binding_->name(id), toJSONStr(record.value(id))) << ',';
        }
        ss_.seekp(-1, std::ios_base::end);  // remove last comma
        ss_ << "}\n";
    }

    exec::OperationPtr source_;
    Format format_;
    std::stringstream ss_;
    ConstFieldBindingPtr binding_;
};

void run(int max_phase, const auto& sources, util::ThreadPool& tp) {
    std::stringstream prof;

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

        if (profile_arg.getValue()) {
            prof << std::format("profile [phase={}]\n{}", phase, exec::prof::Profiler::report());
            exec::prof::Profiler::reset();
        }
    }

    if (profile_arg.getValue()) {
        llog::info("dumping profile");
        std::cerr << prof.str() << std::endl;
    }
}

void explain(int max_phase, const auto& operations) {
    for (int phase = 0; phase <= max_phase; ++phase) {
        std::cout << std::format("===================== planning phase {}", phase) << std::endl;

        exec::Explanation explanation;
        exec::ExplanationCtx ctx{
            .requester = nullptr,
            .phase = phase,
            .explanation = explanation,
        };

        for (auto&& op : operations) {
            auto explain = op->explain(ctx);

            if (!explain.empty()) {
                std::cout << explain.render() << std::endl;
            }
        }

        std::cout << explanation.render() << std::endl;
    }
}

void main(std::span<const char*> argv) {
    if (!parseArgs(argv)) {
        return;
    }

    auto log_level = magic_enum::enum_cast<llog::Level>(log_level_arg.getValue());
    if (!log_level) {
        throw std::runtime_error(
            std::format("invalid value for log-level: {}", log_level_arg.getValue()));
    }
    llog::global()->set_level(static_cast<spdlog::level::level_enum>(*log_level));

    auto format = magic_enum::enum_cast<Format>(format_arg.getValue());
    if (!format) {
        throw std::runtime_error(
            std::format("invalid value for format: {}", format_arg.getValue()));
    }

    std::optional<exec::prof::Profiler> profiler;
    if (profile_arg.getValue()) {
        llog::info("enabling profiling [threads={}]", threads_arg.getValue());
        profiler.emplace(threads_arg.getValue());
    } else {
        llog::info("profiling disabled");
    }

    llog::info("parsing the query and building operations");
    auto [sources, top_operations, binding] = makePlan(sql_file_arg.getValue());

    llog::info("collecting print operations");
    std::vector<std::shared_ptr<Print>> ops;
    for (auto&& [op, fields] : top_operations) {
        ops.push_back(std::make_shared<Print>(op, fields, *format, binding));
    }

    llog::info("determining phase count");
    int max_phase = 0;
    for (auto&& source : sources) {
        max_phase = std::max(max_phase, source->maxPhase());
    }

    if (explain_arg.getValue()) {
        explain(max_phase, ops);
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

#include "exec/op/Operation.h"
#include "exec/prof/Profiler.h"
#include "sql/parser/parse.h"
#include "sql/plan/plan.h"
#include "util/ThreadPool.h"

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

TCLAP::SwitchArg explain_arg{
    "e",
    "explain",
    "show execution plan",
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
    cmd.add(&explain_arg);
    cmd.add(&threads_arg);
    cmd.add(&profile_arg);
    cmd.setExceptionHandling(false);

    try {
        cmd.parse(static_cast<int>(argv.size()), argv.data());
        return true;
    } catch (const TCLAP::ArgException& e) {
        throw std::runtime_error(std::format("error for argument '{}': {}", e.argId(), e.error()));
    } catch (const TCLAP::ExitException& e) {
        return false;
    }
}

std::unique_ptr<sql::ast::Node> parseQuery(std::string maybe_path) {
    std::ifstream ifs;
    std::istream* is = [&] -> std::istream* {
        if (maybe_path.empty()) {
            return &std::cin;
        } else {
            ifs.open(maybe_path.c_str());
            return &ifs;
        }
    }();

    return sql::parse::parse(*is);
}

void printRecordTSKV(const exec::Record::values_t& values, std::stringstream& out) {
    for (auto&& [k, v] : values) {
        out << std::format("{}={}", k, to_string(v)) << '\t';
    }
    out << '\n';
}

std::string toJSONStr(const Value& v) {
    return visit(
        util::Overloaded{
            [](null_t) -> std::string { return "null"; },
            [](bool x) -> std::string { return x ? "true" : "false"; },
            [](int64_t x) -> std::string { return std::to_string(x); },
            [](float x) -> std::string { return std::to_string(x); },
            [](const std::string& x) -> std::string { return std::format("\"{}\"", x); },
        },
        v);
}

void printRecordJSON(const exec::Record::values_t& values, std::stringstream& out) {
    if (values.empty()) {
        out << "{}";
        return;
    }

    out << '{';
    for (auto&& [k, v] : values) {
        out << std::format("\"{}\":{}", k, toJSONStr(v)) << ',';
    }
    out.seekp(-1, std::ios_base::end);  // remove last comma
    out << "}";
}

class Print : public exec::Subscriber {
 public:
    Print(exec::OperationPtr source, Format format) : source_(std::move(source)), format_(format) {
        source_->subscribe(source_->minPhase(), this, exec::RequiredFields::withAll());
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
                printRecordTSKV(record->values(), ss_);
                break;

            case Format::JSON:
                printRecordJSON(record->values(), ss_);
                break;
        }

        return true;
    }

    exec::OperationPtr source_;
    Format format_;
    std::stringstream ss_;
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
                    verify(false, "unhandled exception: {}", e.what());
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
                std::cout << explain.format() << std::endl;
            }
        }

        std::cout << explanation.format() << std::endl;
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

    llog::info("parsing the query");
    auto root = parseQuery(sql_file_arg.getValue());
    verify(root != nullptr);

    std::optional<exec::prof::Profiler> profiler;
    if (profile_arg.getValue()) {
        llog::info("enabling profiling [threads={}]", threads_arg.getValue());
        profiler.emplace(threads_arg.getValue());
    } else {
        llog::info("profiling disabled");
    }

    llog::info("building operations");
    auto [sources, top_operations] = sql::plan::plan(*root);

    llog::info("collecting print operations");
    std::vector<std::shared_ptr<Print>> ops;
    for (auto&& op : top_operations) {
        ops.push_back(std::make_shared<Print>(op, *format));
    }

    llog::info("determining phase count");
    int max_phase = 0;
    for (auto&& source : sources) {
        max_phase = std::max(max_phase, source->maxPhase());
    }

    if (explain_arg.getValue()) {
        explain(max_phase, ops);
    } else {
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

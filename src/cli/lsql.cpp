#include "exec/op/Operation.h"
#include "sql/ast/ExecVisitor.h"
#include "sql/parser/parser.h"

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

void set_parser_context(void* parser, lsql::sql::parse::Context* ctx);
int yylex_init(void** scanner);
void yyset_in(FILE* in, void* scanner);
int yylex(void* scanner);
int yylex_destroy(void* scanner);

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

bool parseArgs(std::span<const char*> argv) {
    TCLAP::CmdLine cmd{"tsql", ' ', "0.0.1"};
    cmd.add(&sql_file_arg);
    cmd.add(&format_arg);
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
    // Initialize Flex scanner
    void* scanner = nullptr;
    yylex_init(&scanner);
    FILE* fd = nullptr;

    if (maybe_path.empty()) {
        yyset_in(stdin, scanner);
    } else {
        FILE* fd = ::fopen(maybe_path.data(), "r");
        assert(fd != nullptr);
        yyset_in(fd, scanner);
    }

    // Initialize Lemon parser
    sql::parse::Context ctx = {nullptr, 0};
    void* parser = ParseAlloc(malloc);

    set_parser_context(parser, &ctx);

    yylex(scanner);
    Parse(parser, 0, {.code = 0, .text = "", .length = 0}, &ctx);

    /* Cleanup */
    yylex_destroy(scanner);
    ParseFree(parser, free);

    if (fd != nullptr) {
        ::fclose(fd);
    }

    if (ctx.has_error) {
        throw std::runtime_error("parsing failed");
    }

    assert(ctx.root);
    return std::move(ctx.root);
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

class Print : public exec::Operation {
 public:
    Print(exec::OperationPtr source, Format format)
        : Operation(1, source->minPhase())
        , source_(std::move(source))
        , format_(format) {}

    void subscribe() { subscribe(source_->minPhase()); }
    void done() const { std::cout << ss_.str() << '\n'; }

 private:
    bool consume(int, const exec::Record* record) {
        if (record == nullptr) {
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

    void subscribe(int phase) override { source_->subscribe(phase, &sub_); }

    exec::OperationPtr source_;
    Format format_;
    exec::MemberSubscriber<Print> sub_{this, &Print::consume};
    std::stringstream ss_;
};

void main(std::span<const char*> argv) {
    if (!parseArgs(argv)) {
        return;
    }

    auto format = magic_enum::enum_cast<Format>(format_arg.getValue());
    if (!format) {
        throw std::runtime_error(
            std::format("invalid value for format: {}", format_arg.getValue()));
    }

    auto root = parseQuery(sql_file_arg.getValue());

    sql::ast::ExecVisitor exec_visitor;
    root->visit(exec_visitor);

    auto sources = std::move(exec_visitor.sources);

    std::vector<std::shared_ptr<Print>> ops;
    while (!exec_visitor.operations.empty()) {
        auto op = std::make_shared<Print>(exec_visitor.popOperation(), *format);
        op->subscribe();
        ops.push_back(op);
    }
    std::ranges::reverse(ops);

    int max_phase = 0;
    for (auto&& source : sources) {
        max_phase = std::max(max_phase, source->maxPhase());
    }

    for (int phase = 0; phase <= max_phase; ++phase) {
        for (auto&& source : sources) {
            if (phase > source->maxPhase()) {
                continue;
            }

            source->push(phase);
        }
    }

    for (auto&& op : ops) {
        op->done();
    }
}

}  // namespace lsql

int main(int argc, const char** argv) {
    try {
        lsql::main(std::span<const char*>(argv, argc));
        return 0;
    } catch (const std::exception& e) {
        std::println("error: {}", e.what());
        return 1;
    } catch (...) {
        std::println("unknown error");
        return 2;
    }
}

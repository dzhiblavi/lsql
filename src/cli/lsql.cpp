#include "sql/ast/ExecVisitor.h"
#include "sql/parser/parser.h"

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

void ParseTrace(FILE* TraceFILE, char* zTracePrompt);
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
    Parse(parser, 0, {.code = 0}, &ctx);  // NOLINT

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

void printRecordTSKV(const rel::Record::values_t& values) {
    for (auto&& [k, v] : values) {
        std::cout << std::format("{}={}", k, to_string(v)) << '\t';
    }
    std::cout << '\n';
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

void printRecordJSON(const rel::Record::values_t& values, std::stringstream& out) {
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

void printRecords(coro::generator<const rel::Record*> records, Format format) {
    switch (format) {
        case Format::TSKV:
            for (auto* record : records) {
                printRecordTSKV(record->values());
            }
            break;

        case Format::JSON:
            std::stringstream ss;
            bool empty = true;
            ss << '[';
            for (auto* record : records) {
                printRecordJSON(record->values(), ss);
                ss << ',';
                empty = false;
            }
            if (!empty) {
                ss.seekp(-1, std::ios_base::end);  // remove last comma
            }
            ss << ']';
            std::cout << ss.str();
            break;
    }
}

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

    std::deque<rel::RelationPtr> relations;
    while (!exec_visitor.relations.empty()) {
        relations.push_front(exec_visitor.popRelation());
    }

    for (auto&& relation : relations) {
        printRecords(relation->records(), *format);
        std::println();
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

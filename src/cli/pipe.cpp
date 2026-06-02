#include "front/pipe/ast/Stringifier.h"
#include "front/pipe/parser/parse.h"
#include "util/require.h"

#include <llog/load.h>
#include <llog/log.h>

#include <magic_enum/magic_enum.hpp>
#include <tclap/CmdLine.h>

#include <fstream>
#include <span>

namespace lsql {

void makePlan(std::string maybe_path) {
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

    auto ast = front::pipe::parse::parse(*is);
    std::cout << "AST dump:" << std::endl;
    std::cout << front::pipe::ast::Stringifier().print(ast) << std::endl;
}

void main(std::span<const char*> /*argv*/) {
    makePlan("");
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

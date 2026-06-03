#include "cli/cli.h"

#include "front/pipe/ast/Stringifier.h"
#include "front/pipe/bind/Statements.h"
#include "front/pipe/bound/Stringifier.h"

#include "front/pipe/lower/Statements.h"
#include "front/pipe/parser/parse.h"

#include "util/require.h"

#include <fstream>

namespace lsql {

ir::Program parseQuery(std::string maybe_path) {
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
    if (print_ast_arg) {
        std::cout << "AST dump:" << std::endl;
        std::cout << front::pipe::ast::Stringifier().print(ast) << std::endl;
    }

    auto bound_ast = front::pipe::bind::bindProgram(std::move(ast));
    if (print_bound_arg) {
        std::cout << "Bound AST dump:" << std::endl;
        std::cout << front::pipe::bound::Stringifier().print(bound_ast) << std::endl;
    }

    return front::pipe::lower::lowerToIR(std::move(bound_ast));
}

}  // namespace lsql

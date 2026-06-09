#include "cli/cli.h"

#include "front/pipe/ast/Stringifier.h"
#include "front/pipe/bind/Statements.h"
#include "front/pipe/bound/Stringifier.h"

#include "front/pipe/lower/Statements.h"
#include "front/pipe/parser/parse.h"

namespace lsql {

std::string_view syntaxName() {
    return "unix pipe style";
}

ir::Program parseQuery(std::string query) {
    auto ast = front::pipe::parse::parse(query);
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

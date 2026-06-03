#include "cli/cli.h"

#include "front/sql/ast/Stringifier.h"
#include "front/sql/bound/Stringifier.h"

#include "front/sql/bind/bind.h"
#include "front/sql/lower/lower.h"
#include "front/sql/parser/parse.h"

#include "util/require.h"

#include <fstream>

namespace lsql {

std::string_view syntaxName() {
    return "SQL style";
}

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

    return front::sql::lower::lowerToIR(std::move(bound_ast));
}

}  // namespace lsql

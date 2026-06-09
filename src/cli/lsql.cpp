#include "cli/cli.h"

#include "front/sql/ast/Stringifier.h"
#include "front/sql/bound/Stringifier.h"

#include "front/sql/bind/bind.h"
#include "front/sql/lower/lower.h"
#include "front/sql/parser/parse.h"

namespace lsql {

std::string_view syntaxName() {
    return "SQL style";
}

ir::Program parseQuery(std::string query) {
    auto ast = front::sql::parse::parse(query);
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

#include "interface/sql/ast/Stringifier.h"
#include "interface/sql/bind/bind.h"
#include "interface/sql/parser/parse.h"

#include "interface/sql/bind/Expressions.h"  // IWYU pragma: keep
#include "interface/sql/bind/Relations.h"    // IWYU pragma: keep
#include "interface/sql/bind/Statement.h"    // IWYU pragma: keep

#include <iostream>
#include <llog/log.h>
#include <magic_enum/magic_enum.hpp>

#include <fstream>
#include <span>

namespace lsql {

void main(std::span<const char*> argv) {
    std::ifstream ifs(argv[1]);
    auto program = iface::sql::parse::parse(ifs);
    std::cout << iface::sql::ast::Stringifier().print(program) << std::endl;

    auto bind = iface::sql::bind::bind(std::move(program));
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

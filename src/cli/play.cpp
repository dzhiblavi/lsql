#include "iface/sql/ast/Stringifier.h"
#include "iface/sql/bound/bind.h"
#include "iface/sql/parser/parse.h"

#include "iface/sql/bound/Expressions.h"  // IWYU pragma: keep
#include "iface/sql/bound/Relations.h"    // IWYU pragma: keep
#include "iface/sql/bound/Statement.h"    // IWYU pragma: keep

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

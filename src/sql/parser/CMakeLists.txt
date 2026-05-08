find_package(magic_enum REQUIRED)

add_library(
    lsql-parser STATIC
    lexer/lex.yy.cpp
    grammar/sql_grammar.cpp
)

target_link_libraries(lsql-parser PRIVATE ReflexLibStatic magic_enum::magic_enum)

find_package(magic_enum REQUIRED)

add_library(
    lsql-parser STATIC
    lexer/lex.yy.cpp
    grammar/sql_grammar.cpp
)

set_source_files_properties(
    lexer/lex.yy.cpp
    grammar/sql_grammar.cpp
    PROPERTIES COMPILE_FLAGS "-w"
)

target_link_libraries(lsql-parser PRIVATE ReflexLibStatic magic_enum::magic_enum)

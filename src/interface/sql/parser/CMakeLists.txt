add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-interface-sql-parser STATIC)
target_sources(lsql-interface-sql-parser PRIVATE parse.cpp)

target_link_libraries(
    lsql-interface-sql-parser PUBLIC
    lsql-interface-sql-parser-grammar
    lsql-interface-sql-parser-lexer
    lsql-core
    ReflexLibStatic
    magic_enum::magic_enum
)

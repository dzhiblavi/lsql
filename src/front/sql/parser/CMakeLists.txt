add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-front-sql-parser STATIC)
target_sources(lsql-front-sql-parser PRIVATE parse.cpp)

target_link_libraries(
    lsql-front-sql-parser PUBLIC
    lsql-front-sql-parser-grammar
    lsql-front-sql-parser-lexer
    lsql-core
    ReflexLibStatic
    magic_enum::magic_enum
)

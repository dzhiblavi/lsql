add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-iface-sql-parser STATIC)
target_sources(lsql-iface-sql-parser PRIVATE parse.cpp)

target_link_libraries(
    lsql-iface-sql-parser PUBLIC
    lsql-iface-sql-parser-grammar
    lsql-iface-sql-parser-lexer
    lsql-core
    ReflexLibStatic
    magic_enum::magic_enum
)

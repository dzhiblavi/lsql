add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-parser STATIC)
target_sources(lsql-parser PRIVATE parse.cpp)

target_link_libraries(
    lsql-parser PUBLIC
    lsql-parser-grammar
    lsql-parser-lexer
    lsql-core
    ReflexLibStatic
    magic_enum::magic_enum
)

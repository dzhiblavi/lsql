add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-front-pipe-parser STATIC)
target_sources(lsql-front-pipe-parser PRIVATE parse.cpp)

target_link_libraries(
    lsql-front-pipe-parser PUBLIC
    lsql-front-pipe-parser-grammar
    lsql-front-pipe-parser-lexer
    lsql-core
    ReflexLibStatic
    magic_enum::magic_enum
)

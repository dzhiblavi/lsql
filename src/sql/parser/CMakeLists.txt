add_subdirectory(grammar)
add_subdirectory(lexer)

find_package(magic_enum REQUIRED)

add_library(lsql-parser INTERFACE)

target_link_libraries(
    lsql-parser INTERFACE
    lsql-parser-grammar
    lsql-parser-lexer
    ReflexLibStatic
    magic_enum::magic_enum
)

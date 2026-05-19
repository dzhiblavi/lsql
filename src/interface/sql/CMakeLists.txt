add_subdirectory(ast)
add_subdirectory(parser)

add_library(lsql-interface-sql STATIC)
target_sources(lsql-interface-sql PRIVATE Interface.cpp)
target_link_libraries(
    lsql-interface-sql PUBLIC
    lsql-interface-sql-ast
    lsql-interface-sql-parser
    lsql-exec
)

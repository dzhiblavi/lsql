add_subdirectory(ast)
add_subdirectory(bind)
add_subdirectory(parser)
add_subdirectory(exec)

add_library(lsql-interface-sql INTERFACE)
#target_sources(lsql-interface-sql PRIVATE Interface.cpp)
target_link_libraries(
    lsql-interface-sql INTERFACE
    lsql-interface-sql-ast
    lsql-interface-sql-parser
    lsql-interface-sql-bind
    lsql-interface-sql-exec
    lsql-exec
)

add_subdirectory(parser)
add_subdirectory(bind)
add_subdirectory(lower)

add_library(lsql-front-sql INTERFACE)

target_link_libraries(
    lsql-front-sql INTERFACE
    lsql-front-sql-parser
    lsql-front-sql-bind
    lsql-front-sql-lower
    lsql-exec
)

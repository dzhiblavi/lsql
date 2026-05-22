add_subdirectory(parser)
add_subdirectory(bind)
add_subdirectory(lower)

add_library(lsql-iface-sql INTERFACE)

target_link_libraries(
    lsql-iface-sql INTERFACE
    lsql-iface-sql-parser
    lsql-iface-sql-bind
    lsql-iface-sql-lower
    lsql-exec
)

add_subdirectory(sql)

add_library(lsql-iface INTERFACE)

target_link_libraries(
    lsql-iface INTERFACE
    lsql-iface-sql
)

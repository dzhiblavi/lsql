add_library(lsql-iface-sql-bind STATIC)

target_sources(
    lsql-iface-sql-bind PRIVATE
    bind.cpp
)

target_link_libraries(
    lsql-iface-sql-bind PUBLIC
    lsql-core
)

add_library(lsql-iface-sql-bind STATIC)
target_sources(
    lsql-iface-sql-bind PRIVATE
    Expressions.cpp
    bind.cpp
)

target_link_libraries(
    lsql-iface-sql-bind PUBLIC
    lsql-core
)

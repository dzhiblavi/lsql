add_library(lsql-iface-sql-lower STATIC)

target_sources(
    lsql-iface-sql-lower PRIVATE
    Expressions.cpp
    Relations.cpp
    Statements.cpp
    lower.cpp
)

target_link_libraries(
    lsql-iface-sql-lower PUBLIC
    lsql-iface-sql-bind
    lsql-core
)

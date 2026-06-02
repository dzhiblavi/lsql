add_library(lsql-front-sql-lower STATIC)

target_sources(
    lsql-front-sql-lower PRIVATE
    Expressions.cpp
    Relations.cpp
    Statements.cpp
    lower.cpp
)

target_link_libraries(
    lsql-front-sql-lower PUBLIC
    lsql-front-sql-bind
    lsql-core
)

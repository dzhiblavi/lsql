add_library(lsql-front-sql-bind STATIC)

target_sources(
    lsql-front-sql-bind PRIVATE
    Expressions.cpp
    Relations.cpp
    Statements.cpp
    bind.cpp
)

target_link_libraries(
    lsql-front-sql-bind PUBLIC
    lsql-core
)

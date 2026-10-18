add_library(lsql-front-sql-bind STATIC)

target_sources(
    lsql-front-sql-bind PRIVATE
    Expressions.cpp
    Relations.cpp
    Statements.cpp
    expand_macros.cpp
    bind.cpp
    post_run.cpp
)

target_link_libraries(
    lsql-front-sql-bind PUBLIC
    lsql-front-common-bind
    lsql-core
)

add_library(lsql-interface-sql-bind STATIC)
target_sources(
    lsql-interface-sql-bind PRIVATE
    Expressions.cpp
    bind.cpp
)

target_link_libraries(
    lsql-interface-sql-bind PUBLIC
    lsql-interface-sql-ast
    lsql-core
)

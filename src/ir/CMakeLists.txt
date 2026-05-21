add_library(lsql-ir STATIC)

target_sources(
    lsql-ir PRIVATE
    Expressions.cpp
)

target_link_libraries(
    lsql-ir PUBLIC
    lsql-core
)

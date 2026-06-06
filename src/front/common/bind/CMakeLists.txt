add_library(lsql-front-common-bind STATIC)

target_sources(
    lsql-front-common-bind PRIVATE
    Expressions.cpp
)

target_link_libraries(
    lsql-front-common-bind PUBLIC
    lsql-core
)

add_library(lsql-front-pipe-lower STATIC)

target_sources(
    lsql-front-pipe-lower PRIVATE
    Expressions.cpp
    Stages.cpp
    Sources.cpp
    Pipeline.cpp
    Statements.cpp
)

target_link_libraries(
    lsql-front-pipe-lower PUBLIC
    lsql-front-pipe-bind
    lsql-core
)

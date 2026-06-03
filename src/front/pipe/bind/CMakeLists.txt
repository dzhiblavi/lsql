add_library(lsql-front-pipe-bind STATIC)

target_sources(
    lsql-front-pipe-bind PRIVATE
    Expressions.cpp
    Stages.cpp
    Sources.cpp
    Pipeline.cpp
    Statements.cpp
)

target_link_libraries(
    lsql-front-pipe-bind PUBLIC
    lsql-core
)

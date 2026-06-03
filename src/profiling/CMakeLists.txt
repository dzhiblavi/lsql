add_library(
    lsql-profiling STATIC
    global.cpp
    Scope.cpp
    presentation.cpp
)

target_link_libraries(
    lsql-profiling PUBLIC
    lsql-util
)

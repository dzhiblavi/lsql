add_library(
    lsql-prof STATIC
    global.cpp
    Scope.cpp
    presentation.cpp
)

target_link_libraries(
    lsql-prof PUBLIC
    lsql-util
)

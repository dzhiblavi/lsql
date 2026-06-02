add_library(
    lsql-back-plan STATIC
    plan.cpp
    GetFileSourceFunc.cpp
    search/SearchRegex.cpp
    search/SearchTimestamp.cpp
)

target_link_libraries(
    lsql-back-plan PUBLIC
    lsql-back-exec
)

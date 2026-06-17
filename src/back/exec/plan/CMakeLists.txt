add_library(lsql-back-exec-plan STATIC)

target_sources(
    lsql-back-exec-plan PRIVATE
    plan.cpp
)

target_link_libraries(
    lsql-back-exec-plan PUBLIC
    lsql-ir
    lsql-util
    lsql-core
)

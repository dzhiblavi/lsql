add_library(
    lsql-exec STATIC
    op/Profiler.cpp
)

target_link_libraries(lsql-exec PUBLIC lsql-core lsql-logs)

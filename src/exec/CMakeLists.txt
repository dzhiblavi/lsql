add_library(
    lsql-exec STATIC
    prof/Profiler.cpp
)

target_link_libraries(lsql-exec PUBLIC lsql-core lsql-logs)

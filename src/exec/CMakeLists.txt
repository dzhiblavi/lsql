add_library(
    lsql-exec INTERFACE
)

target_link_libraries(lsql-exec INTERFACE lsql-core lsql-logs)

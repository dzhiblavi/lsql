add_library(
    lsql-out INTERFACE
)

target_link_libraries(
    lsql-out INTERFACE
    lsql-util
    lsql-core
)

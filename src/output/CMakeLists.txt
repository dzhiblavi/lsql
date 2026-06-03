add_library(
    lsql-output INTERFACE
)

target_link_libraries(
    lsql-output INTERFACE
    lsql-core
    lsql-util
)

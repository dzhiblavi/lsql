find_package(magic_enum REQUIRED)

add_library(
    lsql-logs STATIC
    log_types.cpp
    log_types/tskv2.cpp
    log_types/imap.cpp
)

target_link_libraries(lsql-logs PUBLIC lsql-util lsql-core)

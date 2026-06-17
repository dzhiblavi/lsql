add_library(lsql-back-exec-phys STATIC)

target_sources(
    lsql-back-exec-phys PRIVATE
    build.cpp
    log/log.cpp
    log/search/SearchRegex.cpp
    log/search/SearchTimestamp.cpp
)

target_link_libraries(
    lsql-back-exec-phys PUBLIC
    lsql-back-exec-plan
    lsql-back-storage
    lsql-util
    lsql-core
)

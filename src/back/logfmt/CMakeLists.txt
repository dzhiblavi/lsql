find_package(absl REQUIRED)

add_library(
    lsql-back-logfmt STATIC
    log_types.cpp
)

target_link_libraries(
    lsql-back-logfmt PUBLIC
    lsql-util
    lsql-core
    abseil::abseil
)

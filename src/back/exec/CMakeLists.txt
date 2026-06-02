find_package(absl REQUIRED)

add_library(
    lsql-back-exec INTERFACE
)

target_link_libraries(
    lsql-back-exec INTERFACE
    lsql-back-logfmt
    lsql-core
    lsql-ir
    lsql-prof
    reflectcpp
    libcoro
    llog
    abseil::abseil
)

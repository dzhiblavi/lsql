add_subdirectory(plan)
add_subdirectory(phys)

find_package(absl REQUIRED)

add_library(
    lsql-back-exec INTERFACE
)

target_link_libraries(
    lsql-back-exec INTERFACE
    lsql-back-exec-plan
    lsql-back-exec-phys
    lsql-back-logfmt
    lsql-core
    lsql-ir
    lsql-profiling
    reflectcpp
    libcoro
    abseil::abseil
)

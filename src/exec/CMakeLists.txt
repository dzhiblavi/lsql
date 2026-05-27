find_package(absl REQUIRED)

add_library(
    lsql-exec STATIC
    plan/plan.cpp
    plan/GetFileSourceFunc.cpp
)

target_link_libraries(
    lsql-exec PUBLIC
    lsql-core
    lsql-logs
    lsql-ir
    lsql-prof
    reflectcpp
    libcoro
    llog
    abseil::abseil
)

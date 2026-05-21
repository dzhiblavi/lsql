find_package(absl REQUIRED)

add_library(
    lsql-exec STATIC
    prof/Profiler.cpp
    prof/OperationHandle.cpp
    plan/plan.cpp
    plan/GetFileSourceFunc.cpp
)

target_link_libraries(
    lsql-exec PUBLIC
    lsql-core
    lsql-logs
    lsql-ir
    reflectcpp
    libcoro
    abseil::abseil
)

find_package(absl REQUIRED)

add_library(
    lsql-back-exec STATIC
    plan/plan.cpp
    plan/GetFileSourceFunc.cpp
)

target_link_libraries(
    lsql-back-exec PUBLIC
    lsql-core
    lsql-back-logfmt
    lsql-ir
    lsql-prof
    reflectcpp
    libcoro
    llog
    abseil::abseil
)

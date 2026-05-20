find_package(absl REQUIRED)

add_library(lsql-iface-sql-exec STATIC)
target_sources(
    lsql-iface-sql-exec PRIVATE
    plan.cpp
    GetFileSourceFunc.cpp
)

target_link_libraries(
    lsql-iface-sql-exec PUBLIC
    lsql-iface-sql-bind
    lsql-core
    abseil::abseil
    reflectcpp
    libcoro
)

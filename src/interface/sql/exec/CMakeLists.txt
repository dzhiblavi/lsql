find_package(absl REQUIRED)

add_library(lsql-interface-sql-exec STATIC)
target_sources(
    lsql-interface-sql-exec PRIVATE
    plan.cpp
    GetFileSourceFunc.cpp
)

target_link_libraries(
    lsql-interface-sql-exec PUBLIC
    lsql-interface-sql-bind
    lsql-core
    abseil::abseil
    reflectcpp
    libcoro
)

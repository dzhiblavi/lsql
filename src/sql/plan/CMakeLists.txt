find_package(magic_enum REQUIRED)

add_library(lsql-sql-plan STATIC)
target_sources(lsql-sql-plan PRIVATE plan.cpp)
target_link_libraries(lsql-sql-plan PRIVATE lsql-ast lsql-exec)

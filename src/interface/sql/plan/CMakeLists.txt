add_library(lsql-interface-sql-plan STATIC)
target_sources(lsql-interface-sql-plan PRIVATE plan.cpp FileSourceFunc.cpp)
target_link_libraries(lsql-interface-sql-plan PRIVATE lsql-interface-sql-ast lsql-exec)

add_subdirectory(sql)

add_library(lsql-interface INTERFACE)
target_link_libraries(lsql-interface INTERFACE lsql-interface-sql)

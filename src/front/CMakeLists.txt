add_subdirectory(sql)
#add_subdirectory(pipe)

add_library(lsql-front INTERFACE)

target_link_libraries(
    lsql-front INTERFACE
    lsql-front-sql
    #lsql-front-pipe
)

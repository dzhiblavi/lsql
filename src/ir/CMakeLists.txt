add_library(lsql-ir STATIC)

target_sources(
    lsql-ir PRIVATE
    tools.cpp
)

target_link_libraries(
    lsql-ir PUBLIC
    lsql-core
)

add_library(lsql-ir INTERFACE)

target_sources(
    lsql-ir INTERFACE
)

target_link_libraries(
    lsql-ir INTERFACE
    lsql-core
)

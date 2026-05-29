add_library(lsql-opt STATIC)

target_sources(
    lsql-opt PRIVATE
    optimize.cpp
)

target_link_libraries(
    lsql-opt PUBLIC
    lsql-ir
    lsql-core
)

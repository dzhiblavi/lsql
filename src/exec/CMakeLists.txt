add_library(
    lsql-exec STATIC
    SearchRegex.cpp
    SearchTimestamp.cpp
)

target_link_libraries(lsql-exec PRIVATE lsql-core)

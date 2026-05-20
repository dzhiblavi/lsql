add_library(
    lsql-data STATIC
    PagedFile.cpp
    Page.cpp
    Log.cpp
)

target_link_libraries(
    lsql-data PUBLIC
    lsql-util
    lsql-core
    libcoro
    llog
)

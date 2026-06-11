find_package(ZLIB REQUIRED)

add_library(
    lsql-back-storage STATIC
    Archive.cpp
    PagedFile.cpp
    Page.cpp
    LineSource.cpp
)

target_link_libraries(
    lsql-back-storage PUBLIC
    lsql-util
    lsql-core
    libcoro
    llog
    ZLIB::ZLIB
)

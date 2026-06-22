find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

add_library(
    lsql-back-storage STATIC
    Archive.cpp
    CommandStream.cpp
    PagedFile.cpp
    Page.cpp
    LineSource.cpp
)

target_link_libraries(
    lsql-back-storage PUBLIC
    lsql-util
    lsql-core
    libcoro
    ZLIB::ZLIB
    Threads::Threads
)

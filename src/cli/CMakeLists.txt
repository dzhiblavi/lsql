find_package(tclap REQUIRED)
find_package(magic_enum REQUIRED)

add_executable(tcat tcat.cpp)
target_link_libraries(
    tcat PRIVATE
    tclap::tclap
    lsql-exec
    lsql-data
    lsql-util
    lsql-core
)

target_link_options(tcat PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

add_executable(lsql lsql.cpp)
target_link_libraries(
    lsql PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-exec
    lsql-data
    lsql-util
    lsql-core
    lsql-logs
    lsql-iface-sql
)

target_link_options(lsql PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

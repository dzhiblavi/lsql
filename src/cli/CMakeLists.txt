include(${CMAKE_SOURCE_DIR}/cmake/GenerateDsym.cmake)

find_package(tclap REQUIRED)
find_package(magic_enum REQUIRED)

add_executable(
    lsql
    lsql.cpp
    settings.cpp
)

generate_dsym(lsql)

target_link_libraries(
    lsql PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-front-sql
    lsql-back-exec
    lsql-back-storage
    lsql-back-logfmt
    lsql-optimize
    lsql-core
    lsql-config
    lsql-util
    lsql-profiling
)

target_link_options(
    lsql PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

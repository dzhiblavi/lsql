include(${CMAKE_SOURCE_DIR}/cmake/GenerateDsym.cmake)

find_package(tclap REQUIRED)
find_package(magic_enum REQUIRED)

add_executable(lsql lsql.cpp)
generate_dsym(lsql)
target_link_libraries(
    lsql PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-front-sql
    lsql-back-exec
    lsql-back-plan
    lsql-back-storage
    lsql-back-logfmt
    lsql-optimize
    lsql-core
    lsql-util
)

target_link_options(
    lsql PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

add_executable(lpipe lpipe.cpp)
generate_dsym(lpipe)
target_link_libraries(
    lpipe PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-front-pipe
    lsql-back-exec
    lsql-back-plan
    lsql-back-storage
    lsql-back-logfmt
    lsql-optimize
    lsql-core
    lsql-util
    lsql-front-sql
)

target_link_options(
    lpipe PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

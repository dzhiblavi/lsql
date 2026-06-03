find_package(tclap REQUIRED)
find_package(magic_enum REQUIRED)

add_executable(lsql lsql.cpp)
target_link_libraries(
    lsql PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-back-exec
    lsql-back-plan
    lsql-back-storage
    lsql-back-logfmt
    lsql-util
    lsql-core
    lsql-opt
    lsql-front-sql
)

target_link_options(
    lsql PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

add_executable(lpipe lpipe.cpp)
target_link_libraries(
    lpipe PRIVATE
    tclap::tclap
    magic_enum::magic_enum
    lsql-back-exec
    lsql-back-plan
    lsql-back-storage
    lsql-back-logfmt
    lsql-front-pipe
    lsql-util
    lsql-core
    lsql-opt
    lsql-front-sql
)

target_link_options(
    lpipe PRIVATE
    "-stdlib=libc++"
    "-static-libstdc++"
)

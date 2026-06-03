add_subdirectory(parser)
add_subdirectory(bind)
add_subdirectory(lower)

add_library(lsql-front-pipe INTERFACE)

target_link_libraries(
    lsql-front-pipe INTERFACE
    lsql-front-pipe-parser
    lsql-front-pipe-bind
    lsql-front-pipe-lower
)

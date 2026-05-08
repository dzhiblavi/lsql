find_package(magic_enum REQUIRED)

add_library(
    lsql-util STATIC
    PageSize.cpp
)

target_link_libraries(lsql-util INTERFACE magic_enum::magic_enum)

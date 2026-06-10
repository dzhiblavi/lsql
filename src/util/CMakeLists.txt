find_package(magic_enum REQUIRED)

add_library(
    lsql-util STATIC
    build_info.cpp
    PageSize.cpp
    instrument/duration.cpp
)

target_link_libraries(
    lsql-util PUBLIC
    magic_enum::magic_enum
    llog
    magic_enum::magic_enum
    reflectcpp
)

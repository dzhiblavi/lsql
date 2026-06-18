find_package(magic_enum REQUIRED)

add_library(
    lsql-util STATIC
    archive.cpp
    build_info.cpp
    PageSize.cpp
    thread.cpp
    uniq_id.cpp
    OrderedSink.cpp
    instrument/duration.cpp
)

target_link_libraries(
    lsql-util PUBLIC
    magic_enum::magic_enum
    llog
    magic_enum::magic_enum
    reflectcpp
)

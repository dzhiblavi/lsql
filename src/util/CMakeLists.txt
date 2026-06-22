find_package(spdlog REQUIRED)
find_package(magic_enum REQUIRED)

add_library(
    lsql-util STATIC
    archive.cpp
    build_info.cpp
    PageSize.cpp
    thread.cpp
    uniq_id.cpp
    OrderedSink.cpp
    logging.cpp
    instrument/duration.cpp
)

target_link_libraries(
    lsql-util PUBLIC
    reflectcpp
    magic_enum::magic_enum
    spdlog::spdlog
)

find_package(magic_enum REQUIRED)
find_package(absl REQUIRED)

add_library(
    lsql-core STATIC
    time_formats.cpp
    time_formats/oracle.cpp
    time_formats/iso8601.cpp
    time_formats/sql.cpp
)

target_link_libraries(
    lsql-core PUBLIC
    lsql-util
    ReflexLibStatic
    magic_enum::magic_enum
    llog
    abseil::abseil
)

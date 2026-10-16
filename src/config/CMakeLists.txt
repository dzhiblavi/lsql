add_library(
    lsql-config STATIC
    build_settings.cpp
    dynamic_settings.cpp
)

target_link_libraries(
    lsql-config PUBLIC
    lsql-util
    lsql-core
)

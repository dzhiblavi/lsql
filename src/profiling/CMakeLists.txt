find_package(absl REQUIRED)

add_library(
    lsql-profiling STATIC
    global.cpp
    Scope.cpp
    presentation.cpp
    Profiler.cpp
)

target_link_libraries(
    lsql-profiling PUBLIC
    lsql-util
    abseil::abseil
)

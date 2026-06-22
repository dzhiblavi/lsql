find_package(absl REQUIRED)

add_library(
    lsql-profiling STATIC
    global.cpp
    ScopeBase.cpp
    presentation.cpp
    Profiler.cpp
    Counters.cpp
)

target_link_libraries(
    lsql-profiling PUBLIC
    lsql-util
    abseil::abseil
)
